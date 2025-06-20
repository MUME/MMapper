// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

// FIXME: including display/ from opengl/ is a modularity violation.

#include "Font.h"

#include "../configuration/configuration.h"
#include "../display/Filenames.h"
#include "../display/MapCanvasData.h"
#include "../display/Textures.h"
#include "../global/ConfigConsts.h"
#include "../global/hash.h"
#include "../global/utils.h"
#include "FontFormatFlags.h"
#include "OpenGL.h"

#include <cassert>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QtCore>
#include <QtGui>

static const bool VERBOSE_FONT_DEBUG = []() -> bool {
    if (auto opt = utils::getEnvBool("MMAPPER_VERBOSE_FONT_DEBUG")) {
        return opt.value();
    }
    return false;
}();

// NOTE: Rect doesn't actually include the hi value.
struct NODISCARD Rect final
{
    glm::ivec2 lo = glm::ivec2{0};
    glm::ivec2 hi = glm::ivec2{0};

    NODISCARD int width() const { return hi.x - lo.x; }
    NODISCARD int height() const { return hi.y - lo.y; }
    NODISCARD glm::ivec2 size() const { return {width(), height()}; }
};

NODISCARD static bool intersects(const Rect &a, const Rect &b)
{
#define OVERLAPS(_xy) ((a.lo._xy) < (b.hi._xy) && (b.lo._xy) < (a.hi._xy))
    return OVERLAPS(x) && OVERLAPS(y);
#undef OVERLAPS
}

using IntPair = std::pair<int, int>;

template<>
struct std::hash<IntPair>
{
    std::size_t operator()(const IntPair &ip) const noexcept
    {
#define CAST(i) static_cast<uint64_t>(static_cast<uint32_t>(i))
        return numeric_hash(CAST(ip.first) | (CAST(ip.second) << 32u));
#undef CAST
    }
};

struct NODISCARD FontMetrics final
{
    static constexpr int UNDERLINE_ID = -257;
    static constexpr int BACKGROUND_ID = -258;

    struct NODISCARD Glyph final
    {
        int id = 0;
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        int xoffset = 0;
        int yoffset = 0;
        int xadvance = 0;

        Glyph() = default;
        ~Glyph() = default;
        DEFAULT_CTORS_AND_ASSIGN_OPS(Glyph);

        // used by most cases
        explicit Glyph(const int id_,
                       const int x_,
                       const int y_,
                       const int width_,
                       const int height_,
                       const int xoffset_,
                       const int yoffset_,
                       const int xadvance_)
            : id{id_}
            , x{x_}
            , y{y_}
            , width{width_}
            , height{height_}
            , xoffset{xoffset_}
            , yoffset{yoffset_}
            , xadvance{xadvance_}
        {}

        // used for underline
        explicit Glyph(const int id_,
                       const int x_,
                       const int y_,
                       const int width_,
                       const int height_,
                       const int xoffset_,
                       const int yoffset_)
            : id{id_}
            , x{x_}
            , y{y_}
            , width{width_}
            , height{height_}
            , xoffset{xoffset_}
            , yoffset{yoffset_}
        {}

        // used for background
        explicit Glyph(const int id_, const int x_, const int y_, const int width_, const int height_)
            : id{id_}
            , x{x_}
            , y{y_}
            , width{width_}
            , height{height_}
        {}

        NODISCARD glm::ivec2 getPosition() const { return glm::ivec2{x, y}; }
        NODISCARD glm::ivec2 getSize() const { return glm::ivec2{width, height}; }
        NODISCARD glm::ivec2 getOffset() const { return glm::ivec2{xoffset, yoffset}; }

        NODISCARD Rect getRect() const
        {
            const auto lo = getPosition();
            return Rect{lo, lo + getSize()};
        }
    };

    /// In <a href="https://www.gamedev.net/forums/topic/592614-angelcode-values/?tab=comments#comment-4758799">this
    /// forum post</a>, the Angelcode BMFont author "WitchLord" says: <quote>For example, the kerning pair for the
    /// letters A and T is usually a negative value to make the characters display a bit closer together, while the
    /// kerning pair for the letters A and M is usually a positive value.</quote>
    ///
    /// We know that that BMFont generates `Kerning 65 (aka "A") 84 (aka "T") -1` for `:/fonts/DejaVuSans16.fnt`,
    /// so the amount must be added to the advance / xoffset.
    struct NODISCARD Kerning final
    {
        int first = 0;
        int second = 0;
        int amount = 0;

        Kerning() = default;
        ~Kerning() = default;
        DEFAULT_CTORS_AND_ASSIGN_OPS(Kerning);

        Kerning(const int first_, const int second_, const int amount_)
            : first{first_}
            , second{second_}
            , amount{amount_}
        {}
    };

    struct NODISCARD Common final
    {
        int lineHeight = 0;
        int base = 0;
        int scaleW = 0;
        int scaleH = 0;
        int marginX = 0;
        int marginY = 0;
    };

    std::optional<Glyph> background;
    std::optional<Glyph> underline;

    Common common;

    // REVISIT: Since we only support latin-1, it might make sense to just have fixed
    // size lookup tables such as array<const Glyph*, 256> and array<uint16_t, 65536>
    // for an index into a vector of kernings, rather than using std::unordered_map.
    std::vector<Glyph> raw_glyphs;
    std::vector<Kerning> raw_kernings;
    std::unordered_map<int, const Glyph *> glyphs;
    std::unordered_map<IntPair, const Kerning *> kernings;

    NODISCARD QString init(const QString &);

    NODISCARD const Glyph *lookupGlyph(const int i) const
    {
        const auto it = glyphs.find(i);
        return (it == glyphs.end()) ? nullptr : it->second;
    }

    NODISCARD const Glyph *lookupGlyph(const char c) const
    {
        return lookupGlyph(static_cast<int>(static_cast<unsigned char>(c)));
    }

    NODISCARD const Glyph *getBackground() const
    {
        return background ? &background.value() : nullptr;
    }
    NODISCARD const Glyph *getUnderline() const { return underline ? &underline.value() : nullptr; }

    NODISCARD bool tryAddBackgroundGlyph(QImage &img)
    {
        const int w = common.scaleW;
        const int h = common.scaleH;

        // must not overlap underline
        const Rect ourGlyph{{w - 4, 0}, {w, 4}};

        for (const Glyph &glyph : raw_glyphs) {
            if (intersects(glyph.getRect(), ourGlyph)) {
                qWarning() << "Glyph" << glyph.id << "overlaps expected background location";
                return false;
            }
        }

        if (VERBOSE_FONT_DEBUG) {
            qDebug() << "Adding background glyph";
        }
        // glyph location uses lower left origin
        background.emplace(BACKGROUND_ID, common.scaleW - 3, 1, 2, 2);

        // note: the current image still uses UPPER left origin,
        // but it will be flipped after this function.
        for (int dy = -4; dy < 0; ++dy) {
            for (int dx = -4; dx < 0; ++dx) {
                img.setPixelColor(w + dx, h + dy, Qt::white);
            }
        }
        return true;
    }

    NODISCARD bool tryAddUnderlineGlyph(QImage &img)
    {
        const int w = common.scaleW;
        const int h = common.scaleH;
        const Rect ourGlyph{{w - 12, 0}, {w - 8, 4}};

        // must not overlap background
        for (const Glyph &glyph : raw_glyphs) {
            if (intersects(glyph.getRect(), ourGlyph)) {
                qWarning() << "Glyph" << glyph.id << "overlaps expected underline location";
                return false;
            }
        }

        if (VERBOSE_FONT_DEBUG) {
            qDebug() << "Adding underline glyph";
        }
        // glyph location uses lower left origin
        underline.emplace(UNDERLINE_ID, common.scaleW - 11, 1, 2, 1, 0, -1);

        // note: the current image still uses UPPER left origin,
        // but it will be flipped after this function.
        for (int dy = -4; dy < 0; ++dy) {
            const auto color = (dy == -2) ? QColor{Qt::white} : QColor(0, 0, 0, 0);
            for (int dx = -12; dx < -8; ++dx) {
                img.setPixelColor(w + dx, h + dy, color);
            }
        }
        return true;
    }

    void tryAddSyntheticGlyphs(QImage &img)
    {
        if (img.width() != common.scaleW || img.height() != common.scaleH) {
            qWarning() << "Image is the wrong size";
            return;
        }

        std::ignore = tryAddBackgroundGlyph(img);
        std::ignore = tryAddUnderlineGlyph(img);
    }

    NODISCARD const Kerning *lookupKerning(const Glyph *const prev, const Glyph *const current) const
    {
        if (prev == nullptr || current == nullptr) {
            return nullptr;
        }

        const auto it = kernings.find(IntPair{prev->id, current->id});
        return (it == kernings.end()) ? nullptr : it->second;
    }

    template<typename EmitGlyph>
    void foreach_glyph(const std::string_view msg, EmitGlyph &&emitGlyph) const
    {
        const Glyph *prev = nullptr;
        for (const char &c : msg) {
            const Glyph *const current = lookupGlyph(c);
            if (current != nullptr) {
                emitGlyph(current, lookupKerning(prev, current));
                prev = current;
            } else if (auto oops = lookupGlyph(char_consts::C_QUESTION_MARK)) {
                qWarning() << "Unable to lookup glyph" << QString(QChar(c));
                emitGlyph(oops, lookupKerning(prev, oops));
                prev = oops;
            } else {
                prev = nullptr;
            }
        }
    }

    NODISCARD int measureWidth(const std::string_view msg) const
    {
        int width = 0;
        foreach_glyph(msg, [&width](const Glyph *const g, const Kerning *const k) {
            width += g->xadvance;
            if (k != nullptr) {
                // kerning amount is added to the advance
                width += k->amount;
            }
        });
        return width;
    }

    void getFontBatchRawData(const GLText *text,
                             size_t count,
                             std::vector<FontVert3d> &output) const;
};

// REVISIT: move this to header file?
extern void getFontBatchRawData(const FontMetrics &fm,
                                const GLText *text,
                                size_t count,
                                std::vector<FontVert3d> &output);

void getFontBatchRawData(const FontMetrics &fm,
                         const GLText *const text,
                         const size_t count,
                         std::vector<FontVert3d> &output)
{
    fm.getFontBatchRawData(text, count, output);
}

struct NODISCARD PrintedChar final
{
    int id = 0;
};

QDebug operator<<(QDebug os, PrintedChar c);
QDebug operator<<(QDebug os, PrintedChar c)
{
    os.nospace();
    os << c.id << " (aka " << QString(QChar(c.id)) << ")";
    os.space();
    return os;
}

QString FontMetrics::init(const QString &fontFilename)
{
    qInfo() << "Loading font from " << fontFilename;

    QFile f(fontFilename);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Unable to load font";
        abort();
    }

    raw_glyphs.reserve(256);
    raw_kernings.reserve(1024);

    QFileInfo info(fontFilename);
    QString imageFilename;

    bool hasCommon = false;

    QXmlStreamReader xml(&f);
    while (!xml.atEnd() && !xml.hasError()) {
        if (xml.readNextStartElement()) {
            const auto &attr = xml.attributes();
            if (xml.name() == "common") {
                if (hasCommon) {
                    assert(false);
                    continue;
                }
                hasCommon = true;
                // <common lineHeight="16" base="13" scaleW="256" scaleH="256" pages="1" packed="0" alphaChnl="1" redChnl="0" greenChnl="0" blueChnl="0"/>
                const int lineHeight = attr.value("lineHeight").toInt();
                const int base = attr.value("base").toInt();
                const int scaleW = attr.value("scaleW").toInt();
                const int scaleH = attr.value("scaleH").toInt();
                const int marginX = 2;
                const int marginY = 1;
                if (VERBOSE_FONT_DEBUG) {
                    qDebug() << "Common" << lineHeight << base << scaleW << scaleH << marginX
                             << marginY;
                }
                common = Common{lineHeight, base, scaleW, scaleH, marginX, marginY};

            } else if (xml.name() == "char") {
                if (!hasCommon) {
                    assert(false);
                    continue;
                }

                if (attr.value("page").toInt() != 0 || attr.value("chnl").toInt() != 15) {
                    assert(false);
                    continue;
                }

                // <char id="32" x="197" y="70" width="3" height="1" xoffset="-1" yoffset="15" xadvance="4" page="0" chnl="15" />
                const int id = attr.value("id").toInt();

                const int x = attr.value("x").toInt();
                const int y = attr.value("y").toInt();
                const int width = attr.value("width").toInt();
                const int height = attr.value("height").toInt();
                const int xoffset = attr.value("xoffset").toInt();
                const int yoffset = attr.value("yoffset").toInt();
                const int xadvance = attr.value("xadvance").toInt();

                // REVISIT: should these be offset by -1?
                const int y2 = common.scaleH - (y + height);
                const int yoffset2 = common.base - (yoffset + height);

                if (VERBOSE_FONT_DEBUG) {
                    qDebug() << "Glyph" << PrintedChar{id} << x << y << width << height << xoffset
                             << yoffset << xadvance << "--->" << y2 << yoffset2;
                }

                raw_glyphs.emplace_back(id, x, y2, width, height, xoffset, yoffset2, xadvance);

            } else if (xml.name() == "kerning") {
                if (!hasCommon) {
                    assert(false);
                    continue;
                }

                //   <kerning first="255" second="58" amount="-1" />
                const int first = attr.value("first").toInt();
                const int second = attr.value("second").toInt();
                const int amount = attr.value("amount").toInt();
                if (VERBOSE_FONT_DEBUG) {
                    qDebug() << "Kerning" << PrintedChar{first} << PrintedChar{second} << amount;
                }
                raw_kernings.emplace_back(first, second, amount);
            } else if (xml.name() == "page") {
                const int id = attr.value("id").toInt();
                if (id != 0) {
                    continue;
                }
                const auto &file = attr.value("file").toString();
                const auto &path = info.dir().canonicalPath() + "/" + file;

                const bool exists = QFile{path}.exists();
                if (exists) {
                    imageFilename = path;
                }

                if (VERBOSE_FONT_DEBUG) {
                    QDebug &&os = qDebug();
                    os << "page" << id << file;
                    os.nospace();
                    os << "(aka " << path << ")";
                    os.space();
                    os << (!exists ? "Does not exist." : "Exists.");
                }
            }
        }
    }

    qInfo() << "Loaded" << raw_glyphs.size() << "glyphs and" << raw_kernings.size() << "kernings";

    for (const Glyph &glyph : raw_glyphs) {
        assert(isClamped(glyph.id, 0, 255));
        glyphs[glyph.id] = &glyph;
    }

    for (const Kerning &kerning : raw_kernings) {
        kernings[IntPair{kerning.first, kerning.second}] = &kerning;
    }

    return imageFilename;
}

class NODISCARD FontBatchBuilder final
{
private:
    // only valid as long as GLText it refers to is valid.
    struct NODISCARD Opts final
    {
        std::string_view msg;
        glm::vec3 pos{0.f};
        Color fgColor;
        std::optional<Color> optBgColor;
        bool wantItalics = false;
        bool wantUnderline = false;
        bool wantAlignCenter = false;
        bool wantAlignRight = false;
        int rotationDegrees = 0;
        std::optional<glm::mat4> rotation = glm::mat4(1.f);

        explicit Opts() = default;

        void reset() { *this = Opts{}; }

        explicit Opts(const GLText &text)
            : msg{text.text}
            , pos{text.pos}
            , fgColor{text.color}
            , optBgColor{text.bgcolor}
            , wantItalics{text.fontFormatFlag.contains(FontFormatFlagEnum::ITALICS)}
            , wantUnderline{text.fontFormatFlag.contains(FontFormatFlagEnum::UNDERLINE)}
            , wantAlignCenter{text.fontFormatFlag.contains(FontFormatFlagEnum::HALIGN_CENTER)}
            , wantAlignRight{text.fontFormatFlag.contains(FontFormatFlagEnum::HALIGN_RIGHT)}
            , rotationDegrees{text.rotationAngle}
            , rotation{(rotationDegrees == 0)
                           ? std::optional<glm::mat4>(std::nullopt)
                           : std::optional<glm::mat4>(
                               glm::rotate(glm::mat4(1),
                                           glm::radians(static_cast<float>(rotationDegrees)),
                                           glm::vec3(0, 0, 1)))}
        {}
    };

    struct NODISCARD Bounds final
    {
        glm::ivec2 maxVertPos{};
        glm::ivec2 minVertPos{};
        void include(const glm::ivec2 &vertPos)
        {
            minVertPos = glm::min(vertPos, minVertPos);
            maxVertPos = glm::max(vertPos, maxVertPos);
        }
    };

private:
    const FontMetrics &m_fm;
    const glm::ivec2 m_iTexSize{};
    std::vector<FontVert3d> &m_verts3d;
    Opts m_opts;
    Bounds m_bounds;
    int m_xlinepos = 0;
    bool m_noOutput = false;
    void resetPerStringData()
    {
        m_opts.reset();
        m_bounds = Bounds();
        m_xlinepos = 0;
        m_noOutput = false;
    }

public:
    explicit FontBatchBuilder(const FontMetrics &fm, std::vector<FontVert3d> &output)
        : m_fm{fm}
        , m_iTexSize{fm.common.scaleW, fm.common.scaleH}
        , m_verts3d{output}
    {}

    NODISCARD glm::vec2 getTexCoord(const glm::ivec2 &iTexCoord) const
    {
        return glm::vec2(iTexCoord) / glm::vec2(m_iTexSize);
    }

    // REVISIT: This could be done in the shader,
    // at the cost of transmitting italics bit and rotation angle.
    NODISCARD glm::vec2 transformVert(const glm::ivec2 &ipos) const
    {
        glm::vec2 pos(ipos);

        if (m_opts.wantItalics) {
            pos.x += pos.y / 6.f;
        }

        if (m_opts.rotation) {
            pos = glm::vec2(m_opts.rotation.value() * glm::vec4(pos, 0, 1));
        }
        return pos;
    }

    void emitGlyphQuad(const bool isEmpty,
                       const glm::ivec2 &iVertex00,
                       const glm::ivec2 &iTexCoord00,
                       const glm::ivec2 &iglyphSize)
    {
        const auto emitWithOffset =
            [this, isEmpty, &iVertex00, &iTexCoord00](const glm::ivec2 &pixelOffset) -> void {
            const glm::ivec2 relativeVertPos = iVertex00 + pixelOffset;
            if (!isEmpty) {
                // side-effect: updates bounds; this must come before return
                m_bounds.include(relativeVertPos);
            }

            if (m_noOutput) {
                return;
            }

            const glm::vec2 tc = getTexCoord(iTexCoord00 + pixelOffset);
            const glm::vec2 vert = transformVert(relativeVertPos);
            m_verts3d.emplace_back(m_opts.pos, m_opts.fgColor, tc, vert);
        };

        const auto &x = iglyphSize.x;
        const auto &y = iglyphSize.y;
        // 3-2
        // | |
        // 0-1
        emitWithOffset(glm::ivec2(0, 0));
        emitWithOffset(glm::ivec2(x, 0));
        emitWithOffset(glm::ivec2(x, y));
        emitWithOffset(glm::ivec2(0, y));
    }

    void emitGlyph(const FontMetrics::Glyph *const g, const FontMetrics::Kerning *const k)
    {
        assert(isClamped(g->id, 0, 255));
        const auto glyphSize = glm::ivec2(g->width, g->height);
        const auto iTexCoord00 = glm::ivec2(g->x, g->y);
        if (k != nullptr) {
            // kerning amount is added to the advance
            m_xlinepos += k->amount;
        }
        const auto iVertex00 = glm::ivec2(m_xlinepos + g->xoffset, g->yoffset);
        m_xlinepos += g->xadvance;
        emitGlyphQuad(std::isspace(g->id), iVertex00, iTexCoord00, glyphSize);
    }

    void call_foreach_glyph(const int wordOffset, const bool output)
    {
        const auto emitGlyphLambda = [this](const FontMetrics::Glyph *const g,
                                            const FontMetrics::Kerning *const k) {
            emitGlyph(g, k);
        };

        m_noOutput = !output;
        m_xlinepos = wordOffset;
        m_fm.foreach_glyph(m_opts.msg, emitGlyphLambda);
    }

    void addString(const GLText &text)
    {
        resetPerStringData();
        this->m_opts = Opts{text};

        int wordOffset = 0;
        call_foreach_glyph(wordOffset, false);

        // measurement, background color, and underline.
        {
            const auto add = [this](const Color &c, const glm::ivec2 &ivert, const glm::ivec2 &itc) {
                const glm::vec2 tc = getTexCoord(itc);
                const glm::vec2 vert = transformVert(ivert);
                m_verts3d.emplace_back(m_opts.pos, c, tc, vert);
            };

            const auto quad = [&add](const Color &c, const Rect &vert, const Rect &tc) {
#define ADD(a, b) add(c, glm::ivec2{vert.a.x, vert.b.y}, glm::ivec2{tc.a.x, tc.b.y})
                // note: lo and hi refer to members of vert and tc.
                ADD(lo, lo);
                ADD(hi, lo);
                ADD(hi, hi);
                ADD(lo, hi);
#undef ADD
            };

            const glm::ivec2 margin{m_fm.common.marginX, m_fm.common.marginY};
            const auto &lo = m_bounds.minVertPos;
            const auto &hi = m_bounds.maxVertPos;

            if (m_opts.wantAlignCenter) {
                const auto halfWidth = m_xlinepos / 2;
                wordOffset -= halfWidth;
                m_bounds.minVertPos.x -= halfWidth;
                m_bounds.maxVertPos.x -= halfWidth;
            } else if (m_opts.wantAlignRight) {
                wordOffset -= m_xlinepos;
                m_bounds.minVertPos.x -= m_xlinepos;
                m_bounds.maxVertPos.x -= m_xlinepos;
            }

            if (m_opts.optBgColor) {
                if (const FontMetrics::Glyph *const background = m_fm.getBackground()) {
                    quad(m_opts.optBgColor.value(),
                         Rect{lo - margin, hi + margin},
                         background->getRect());
                }
            }

            if (m_opts.wantUnderline) {
                if (const FontMetrics::Glyph *const underline = m_fm.getUnderline()) {
                    const auto usize = underline->getSize();
                    const auto offset = underline->getOffset() + glm::ivec2{wordOffset, 0};
                    quad(m_opts.fgColor,
                         Rect{offset, offset + glm::ivec2{m_xlinepos, usize.y}},
                         underline->getRect());
                }
            }
        }

        // note: 2nd call includes wordOffset that can be modified
        // above if caller requested HALIGN_CENTER or HALIGN_RIGHT.
        call_foreach_glyph(wordOffset, true);
    }
};

GLFont::GLFont(OpenGL &gl)
    : m_gl(gl)
{}

GLFont::~GLFont() = default;

NODISCARD static QString getFontFilename(const float devicePixelRatio)
{
    const char *const FONT_KEY = "MMAPPER_FONT";
    const char *const font = "Cantarell";
    const char *const size = [&devicePixelRatio]() {
        if (devicePixelRatio > 1.75f) {
            return "36";
        }
        if (devicePixelRatio > 1.25f) {
            return "27";
        }
        return "18";
    }();
    const QString fontFilename = QString(":/fonts/%1%2.fnt").arg(font).arg(size);
    if (qEnvironmentVariableIsSet(FONT_KEY)) {
        const QString tmp = qgetenv(FONT_KEY);
        if (QFile{tmp}.exists()) {
            qInfo() << "Using value from" << FONT_KEY << "to override font from" << fontFilename
                    << "to" << tmp;
            return tmp;
        } else {
            qInfo() << "Path in" << FONT_KEY << "is invalid.";
        }
    } else if (IS_DEBUG_BUILD) {
        qInfo() << "Note: You can override the font with" << FONT_KEY;
    }

    if (!QFile{fontFilename}.exists()) {
        qWarning() << fontFilename << "does not exist.";
    }

    return fontFilename;
}

void GLFont::init()
{
    assert(m_gl.isRendererInitialized());
    auto pfm = std::make_shared<FontMetrics>();
    m_fontMetrics = pfm;
    const auto fontFilename = getFontFilename(m_gl.getDevicePixelRatio());

    auto &fm = *pfm;
    const QString imageFilename = fm.init(fontFilename);

    if (!QFile{imageFilename}.exists()) {
        qWarning() << "invalid font filename" << imageFilename;
    }

    if (m_texture) {
        m_texture->clearId();
    }

    // REVISIT: can this avoid switching to a different MMTexture object?
    m_texture = MMTexture::alloc(
        QOpenGLTexture::Target::Target2D,
        [&fm, &imageFilename](QOpenGLTexture &tex) -> void {
            QImage img{imageFilename};
            fm.tryAddSyntheticGlyphs(img);
            img = img.mirrored();

            tex.setMinMagFilters(QOpenGLTexture::Filter::Linear, QOpenGLTexture::Filter::Linear);
            tex.setAutoMipMapGenerationEnabled(false);
            tex.setMipLevels(0);
            tex.setData(img, QOpenGLTexture::MipMapGeneration::DontGenerateMipMaps);
        },
        true);

    // Each new MMTexture gets assigned the same old ID.

    m_texture->setId(m_id);
    m_gl.setTextureLookup(m_id, m_texture);
}

void GLFont::cleanup()
{
    m_fontMetrics.reset();
    m_texture.reset();
}

int GLFont::getFontHeight() const
{
    return getFontMetrics().common.lineHeight;
}

std::optional<int> GLFont::getGlyphAdvance(const char c) const
{
    if (const FontMetrics::Glyph *const g = getFontMetrics().lookupGlyph(c)) {
        return g->xadvance;
    }
    return std::nullopt;
}

glm::ivec2 GLFont::getScreenCenter() const
{
    return m_gl.getPhysicalViewport().offset + m_gl.getPhysicalViewport().size / 2;
}

void FontMetrics::getFontBatchRawData(const GLText *const text,
                                      const size_t count,
                                      std::vector<FontVert3d> &output) const
{
    if (count == 0) {
        return;
    }

    const auto before = output.size();
    const auto end = text + count;

    const size_t expectedVerts = [text, end]() -> size_t {
        int numGlyphs = 0;
        for (const GLText *it = text; it != end; ++it) {
            numGlyphs += static_cast<int>(it->text.size()) + (it->bgcolor.has_value() ? 1 : 0)
                         + (it->fontFormatFlag.contains(FontFormatFlagEnum::UNDERLINE) ? 1 : 0);
        }
        return 4 * static_cast<size_t>(numGlyphs);
    }();

    output.reserve(before + expectedVerts);

    auto &fm = *this;
    FontBatchBuilder fontBatchBuilder{fm, output};
    for (const GLText *it = text; it != end; ++it) {
        fontBatchBuilder.addString(*it);
    }
    assert(output.size() == before + expectedVerts);
}

void GLFont::render2dTextImmediate(const std::vector<GLText> &text)
{
    if (text.empty()) {
        return;
    }

    // input position: physical pixels, with origin at upper left
    // output: [-1, 1]^2
    const auto vp = m_gl.getPhysicalViewport();
    const auto viewProj = glm::scale(glm::mat4(1), glm::vec3(2, 2, 1))
                          * glm::translate(glm::mat4(1), glm::vec3(-0.5f, 0.5, 0))
                          * glm::scale(glm::mat4(1),
                                       glm::vec3(1.f / glm::vec2(vp.size.x, -vp.size.y), 1.f))
                          * glm::translate(glm::mat4(1), glm::vec3(-glm::vec2(vp.offset), 1.f));

    const auto oldProj = m_gl.getProjectionMatrix();
    m_gl.setProjectionMatrix(viewProj);
    render3dTextImmediate(text);
    m_gl.setProjectionMatrix(oldProj);
}

void GLFont::render3dTextImmediate(const std::vector<FontVert3d> &rawVerts)
{
    if (rawVerts.empty()) {
        return;
    }

    m_gl.renderFont3d(m_texture, rawVerts);
}

void GLFont::render3dTextImmediate(const std::vector<GLText> &text)
{
    if (text.empty()) {
        return;
    }

    const auto rawVerts = getFontMeshIntermediate(text);
    render3dTextImmediate(rawVerts);
}

std::vector<FontVert3d> GLFont::getFontMeshIntermediate(const std::vector<GLText> &text)
{
    std::vector<FontVert3d> output;
    getFontMetrics().getFontBatchRawData(text.data(), text.size(), output);
    return output;
}

UniqueMesh GLFont::getFontMesh(const std::vector<FontVert3d> &rawVerts)
{
    return m_gl.createFontMesh(m_texture, DrawModeEnum::QUADS, rawVerts);
}

void GLFont::renderTextCentered(const QString &text,
                                const Color &color,
                                const std::optional<Color> &bgcolor)
{
    // here we're converting to latin1 because we cannot display unicode codepoints above 255
    const auto center = glm::vec2{getScreenCenter()};
    render2dTextImmediate(
        std::vector<GLText>{GLText{glm::vec3{center, 0.f},
                                   mmqt::toStdStringLatin1(text), // GL font is latin1
                                   color,
                                   bgcolor,
                                   FontFormatFlags{FontFormatFlagEnum::HALIGN_CENTER}}});
}
