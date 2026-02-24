// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Textures.h"

#include "../configuration/configuration.h"
#include "../global/TextUtils.h"
#include "../global/thread_utils.h"
#include "../global/utils.h"
#include "../opengl/OpenGLTypes.h"
#include "Filenames.h"
#include "RoadIndex.h"
#include "mapcanvas.h"

#include <algorithm>
#include <array>
#include <map>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#include <glm/glm.hpp>

#include <QMessageLogContext>

MMTextureId allocateTextureId()
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    static MMTextureId next{0};
    return next++;
}

namespace { // anonymous
struct NODISCARD Measurements final
{
    int max_input_w = 0;
    int max_input_h = 0;
    int total_layers = 0;
    int max_manual_mip_levels = 0;
    bool has_file = false;
    bool has_image = false;
    std::map<int, int> counts;

    void add(const std::string_view groupName, const SharedMMTexture &pTex)
    {
        const MMTexture &tex = deref(pTex);
        const QOpenGLTexture &qtex = deref(tex.get());

        const int w = qtex.width();
        const int h = qtex.height();
        const int s = std::max(w, h);

        max_input_w = std::max(max_input_w, w);
        max_input_h = std::max(max_input_h, h);
        total_layers += 1;

        const bool useImages = tex.getName().isEmpty();
        if (useImages) {
            has_image = true;
            const int numImages = static_cast<int>(pTex->getImages().size());
            max_manual_mip_levels = std::max(max_manual_mip_levels, numImages);
        } else {
            has_file = true;
        }

        const int nearest = static_cast<int>(utils::nearestPowerOfTwo(static_cast<uint32_t>(s)));
        counts[nearest]++;

        const std::string name = mmqt::toStdStringUtf8(tex.getName());
        if (w != h) {
            MMLOG_WARNING() << "[Textures] Warning in group '" << groupName << "': Image '" << name
                            << "' is not square (" << w << "x" << h << ").";
            if (useImages) {
                throw std::runtime_error("image must be square");
            }
        }
        if (!utils::isPowerOfTwo(static_cast<uint32_t>(w))
            || !utils::isPowerOfTwo(static_cast<uint32_t>(h))) {
            MMLOG_WARNING() << "[Textures] Warning in group '" << groupName << "': Image '" << name
                            << "' is not a power of two (" << w << "x" << h << ").";
            if (useImages) {
                throw std::runtime_error("image size must be a power of two");
            }
        }
    }

    NODISCARD int getWinner() const
    {
        int winner = 0;
        int maxCount = 0;
        for (auto const &[size, count] : counts) {
            if (count > maxCount || (count == maxCount && size > winner)) {
                winner = size;
                maxCount = count;
            }
        }
        return winner;
    }

    void validate(const std::string_view groupName) const
    {
        if (total_layers == 0) {
            throw std::runtime_error("Empty texture group: " + std::string(groupName));
        }
        if (has_file && has_image) {
            throw std::runtime_error("Mixed file and image textures in group: "
                                     + std::string(groupName));
        }
    }

    void log(const std::string_view groupName, const int winner) const
    {
        auto &&os = MMLOG();
        os << "[initTextures] Picking " << winner << "x" << winner << " for array group '"
           << groupName << "' (distribution: ";
        bool first = true;
        for (auto const &[size, count] : counts) {
            if (!first) {
                os << ", ";
            }
            os << size << "x" << size << ":" << count;
            first = false;
        }
        os << ")\n";
    }
};
} // namespace

MMTexture::MMTexture(Badge<MMTexture>, const QString &name)
    : m_qt_texture{QOpenGLTexture::Target2D}
    , m_name{name}
    , m_sourceData{std::make_unique<SourceData>()}
{
    auto &tex = m_qt_texture;
    QImage image{name};
    if (!image.isNull()) {
        const QImage converted = image.mirrored().convertToFormat(QImage::Format_RGBA8888);
        tex.setFormat(QOpenGLTexture::TextureFormat::RGBA8_UNorm);
        tex.setSize(converted.width(), converted.height());
        tex.setMipLevels(tex.maximumMipLevels());
        tex.allocateStorage();
        tex.setData(0,
                    QOpenGLTexture::PixelFormat::RGBA,
                    QOpenGLTexture::PixelType::UInt8,
                    converted.constBits());
        tex.generateMipMaps();
    }
    tex.setWrapMode(QOpenGLTexture::WrapMode::MirroredRepeat);
    tex.setMinMagFilters(QOpenGLTexture::Filter::LinearMipMapLinear, QOpenGLTexture::Filter::Linear);
}

MMTexture::MMTexture(Badge<MMTexture>, std::vector<QImage> images)
    : m_qt_texture{QOpenGLTexture::Target2D}
    , m_sourceData{std::make_unique<SourceData>(std::move(images))}
{
    if (m_sourceData->m_images.empty()) {
        throw std::logic_error("cannot construct MMTexture from empty vector of images");
    }

    const auto &front = m_sourceData->m_images.front();
    size_t level = 0;
    for (const auto &im : m_sourceData->m_images) {
        assert(im.width() == im.height());
        assert(im.width() == (front.width() >> level));
        assert(im.height() == (front.height() >> level));
        ++level;
    }

    auto &tex = m_qt_texture;
    tex.setFormat(QOpenGLTexture::TextureFormat::RGBA8_UNorm);
    const int numLevels = static_cast<int>(level);
    tex.setMipLevels(numLevels);
    tex.setMipMaxLevel(numLevels - 1);
    tex.setAutoMipMapGenerationEnabled(false);
    tex.setWrapMode(QOpenGLTexture::WrapMode::MirroredRepeat);
    tex.setMinMagFilters(QOpenGLTexture::Filter::NearestMipMapNearest,
                         QOpenGLTexture::Filter::Nearest);
    tex.setSize(front.width(), front.height());
    tex.allocateStorage();

    for (int i = 0; i < numLevels; ++i) {
        const QImage img = m_sourceData->m_images[static_cast<size_t>(i)].convertToFormat(
            QImage::Format_RGBA8888);
        tex.setData(i,
                    QOpenGLTexture::PixelFormat::RGBA,
                    QOpenGLTexture::PixelType::UInt8,
                    img.constBits());
    }
}

void MapCanvasTextures::destroyAll()
{
    for_each([](SharedMMTexture &tex) -> void { tex.reset(); });

    static constexpr bool verbose_logging = IS_DEBUG_BUILD;

    // REVISIT: what happens to this MMLOG() in non-debug build, since it doesn't print anything?
    auto &&os = MMLOG();
    const auto reset_one_array = [&os](const std::string_view name, SharedMMTexture &tex) {
        if constexpr (verbose_logging) {
            const QOpenGLTexture &qtex = deref(deref(tex).get());
            const auto layers = qtex.layers();
            os << "... " << name;
            os << " w/ " << layers << " layer" << ((layers == 1) ? "" : "s");
            os << " at " << qtex.width() << "x" << qtex.height();
            os << "\n";
        }
        tex.reset();
    };

    if constexpr (verbose_logging) {
        os << "destroying...\n";
    }
    for_each_array(reset_one_array);
    if constexpr (verbose_logging) {
        os << "Done\n";
    }
}

NODISCARD static SharedMMTexture loadTexture(const QString &name)
{
    auto mmtex = MMTexture::alloc(name);
    auto *texture = mmtex->get();
    if (!texture->isCreated()) {
        qWarning() << "failed to create: " << name;
        texture->setSize(1);
        texture->create();

        if (!texture->isCreated()) {
            throw std::runtime_error(mmqt::toStdStringUtf8("failed to create: " + name));
        }
    }

    texture->setWrapMode(QOpenGLTexture::WrapMode::MirroredRepeat);
    return mmtex;
}

template<typename E>
static void loadPixmapArray(texture_array<E> &textures)
{
    const auto N = textures.size();
    for (size_t i = 0u; i < N; ++i) {
        const auto x = static_cast<E>(i);
        textures[x] = loadTexture(getPixmapFilename(x));
    }
}

template<RoadTagEnum Tag>
static void loadPixmapArray(road_texture_array<Tag> &textures)
{
    const auto N = textures.size();
    for (size_t i = 0u; i < N; ++i) {
        const auto x = TaggedRoadIndex<Tag>{static_cast<RoadIndexMaskEnum>(i)};
        textures[x] = loadTexture(getPixmapFilename(x));
    }
}

// Technically only the "minifying" filter can be trilinear.
//
// GL_NEAREST = 1 sample from level 0 (no mipmapping).
// GL_LINEAR = 4 samples from level 0 (no mipmapping).
//
// GL_NEAREST_MIPMAP_NEAREST = 1 sample (nearest mip).
// GL_NEAREST_MIPMAP_LINEAR = 2 samples (samples 2 nearest mips).
//
// GL_LINEAR_MIPMAP_NEAREST = 4 samples (nearest mip).
// GL_LINEAR_MIPMAP_LINEAR = 8 samples (trilinear).
//
static void setTrilinear(const SharedMMTexture &mmtex, const bool trilinear)
{
    if (mmtex == nullptr || !mmtex->canBeUpdated()) {
        return;
    }

    if (QOpenGLTexture *const qtex = mmtex->get()) {
        qtex->setMinMagFilters(
            /* "minifying" filter */
            trilinear ? QOpenGLTexture::LinearMipMapLinear /* 8 samples */
                      : QOpenGLTexture::NearestMipMapLinear /* 2 samples (default) */,
            /* magnifying filter */
            QOpenGLTexture::Linear /* 4 samples (default) */);
    }
}

NODISCARD static std::vector<QImage> createDottedWallImages(const ExitDirEnum dir)
{
    static constexpr uint32_t MAX_BITS = 7;
    // MAYBE_UNUSED static constexpr int SIZE = 1 << MAX_BITS;

    const QColor OPAQUE_WHITE = Qt::white;
    const QColor TRANSPARENT_BLACK = QColor::fromRgbF(0.0, 0.0, 0.0, 0.0);
    std::vector<QImage> images;
    images.reserve(MAX_BITS + 1);

    for (auto i = 0u; i <= MAX_BITS; ++i) {
        const int size = 1 << (MAX_BITS - i);
        QImage &image = images.emplace_back(size, size, QImage::Format::Format_RGBA8888);
        image.fill(TRANSPARENT_BLACK);

        if (size >= 16) {
            // 64 and 128:
            // ##..##..##..##..##..##..##..##..##..##..##..##..##..##..##..##..
            // ##..##..##..##..##..##..##..##..##..##..##..##..##..##..##..##..
            // ##..##..##..##..##..##..##..##..##..##..##..##..##..##..##..##..
            // ##..##..##..##..##..##..##..##..##..##..##..##..##..##..##..##..
            // 32:
            // ##..##..##..##..##..##..##..##..
            // ##..##..##..##..##..##..##..##..
            // 16:
            // ##..##..##..##..

            const int width = std::invoke([i]() -> int {
                switch (MAX_BITS - i) {
                case 4:
                    return 1;
                case 5:
                    return 2;
                case 6:
                case 7:
                    return 4;
                default:
                    assert(false);
                    return 4;
                }
            });

            assert(isClamped(width, 1, 4));

            for (int y = 0; y < width; ++y) {
                for (int x = 0; x < size; x += 4) {
                    image.setPixelColor(x + 0, y, OPAQUE_WHITE);
                    image.setPixelColor(x + 1, y, OPAQUE_WHITE);
                }
            }
        } else if (size == 8) {
            // #...#...
            image.setPixelColor(1, 0, OPAQUE_WHITE);
            image.setPixelColor(5, 0, OPAQUE_WHITE);
        } else if (size == 4) {
            // -.-.
            image.setPixelColor(0, 0, QColor::fromRgbF(1.0, 1.0, 1.0, 0.5));
            image.setPixelColor(2, 0, QColor::fromRgbF(1.0, 1.0, 1.0, 0.5));
        } else if (size == 2) {
            // ..
            image.setPixelColor(0, 0, QColor::fromRgbF(1.0, 1.0, 1.0, 0.25));
            image.setPixelColor(1, 0, QColor::fromRgbF(1.0, 1.0, 1.0, 0.25));
        }

        if (dir == ExitDirEnum::EAST || dir == ExitDirEnum::WEST) {
            const auto halfSize = static_cast<double>(size) * 0.5;
            QTransform matrix;
            matrix.translate(halfSize, halfSize);
            matrix.rotate(90);
            matrix.translate(-halfSize, -halfSize);
            image = std::move(image).transformed(matrix, Qt::FastTransformation);
        }

        if (dir == ExitDirEnum::NORTH || dir == ExitDirEnum::WEST) {
            image = std::move(image).mirrored(true, true);
        }
    }
    return images;
}

template<typename Type>
static void appendAll(std::vector<SharedMMTexture> &v, Type &&thing)
{
    if constexpr (std::is_same_v<std::decay_t<Type>, SharedMMTexture>) {
        v.emplace_back(std::forward<Type>(thing));
    } else {
        for (const SharedMMTexture &t : thing) {
            v.emplace_back(t);
        }
    }
}

template<typename... Types>
NODISCARD static std::vector<SharedMMTexture> combine(Types &&...things)
{
    std::vector<SharedMMTexture> tmp;
    (appendAll(tmp, things), ...);
    return tmp;
}

void MapCanvas::initTextures()
{
    MapCanvasTextures &textures = this->m_textures;
    auto &opengl = this->getOpenGL();

    loadPixmapArray(textures.terrain); // 128
    loadPixmapArray(textures.road);    // 128
    loadPixmapArray(textures.trail);   // 64
    loadPixmapArray(textures.mob);     // 128
    loadPixmapArray(textures.load);    // 128

    for (const ExitDirEnum dir : ALL_EXITS_NESW) {
        textures.dotted_wall[dir] = MMTexture::alloc(createDottedWallImages(dir));
        textures.wall[dir] = loadTexture(
            getPixmapFilenameRaw(QString::asprintf("wall-%s.png", lowercaseDirection(dir))));
    }
    for (const ExitDirEnum dir : ALL_EXITS_NESWUD) {
        // doors are 256
        textures.door[dir] = loadTexture(
            getPixmapFilenameRaw(QString::asprintf("door-%s.png", lowercaseDirection(dir))));
        // streams are 128
        textures.stream_in[dir] = loadTexture(
            getPixmapFilenameRaw(QString::asprintf("stream-in-%s.png", lowercaseDirection(dir))));
        textures.stream_out[dir] = loadTexture(
            getPixmapFilenameRaw(QString::asprintf("stream-out-%s.png", lowercaseDirection(dir))));
    }
    {
        // 1x1
        QImage whitePixel(1, 1, QImage::Format_RGBA8888);
        whitePixel.fill(Qt::white);
        textures.white_pixel = MMTexture::alloc(std::vector<QImage>{whitePixel});
    }

    // char images are 256
    textures.char_arrows = loadTexture(getPixmapFilenameRaw("char-arrows.png"));
    textures.char_room_sel = loadTexture(getPixmapFilenameRaw("char-room-sel.png"));
    // exits are 128
    textures.exit_climb_down = loadTexture(getPixmapFilenameRaw("exit-climb-down.png"));
    textures.exit_climb_up = loadTexture(getPixmapFilenameRaw("exit-climb-up.png"));
    textures.exit_down = loadTexture(getPixmapFilenameRaw("exit-down.png"));
    textures.exit_up = loadTexture(getPixmapFilenameRaw("exit-up.png"));
    textures.no_ride = loadTexture(getPixmapFilenameRaw("no-ride.png"));
    // room are 256
    textures.room_sel = loadTexture(getPixmapFilenameRaw("room-sel.png"));
    textures.room_sel_distant = loadTexture(getPixmapFilenameRaw("room-sel-distant.png"));
    textures.room_sel_move_bad = loadTexture(getPixmapFilenameRaw("room-sel-move-bad.png"));
    textures.room_sel_move_good = loadTexture(getPixmapFilenameRaw("room-sel-move-good.png"));
    // 256
    textures.room_highlight = loadTexture(getPixmapFilenameRaw("room-highlight.png"));

    auto assignId = [this](SharedMMTexture &pTex) -> MMTextureId {
        auto &tex = deref(pTex);
        assert(tex.get()); // make sure we didn't forget to initialize one

        auto id = allocateTextureId();
        tex.setId(id);
        m_opengl.setTextureLookup(id, pTex);
        return id;
    };

    Measurements global_m;
    textures.for_each([&assignId, &global_m](SharedMMTexture &pTex) {
        MAYBE_UNUSED auto ignored = assignId(pTex);
        global_m.add("global", pTex);
    });

    {
        auto maybeCreateArray2 = [&assignId, &opengl](const std::string_view groupName,
                                                      const auto &thing,
                                                      SharedMMTexture &pArrayTex) {
            Measurements group_m;
            for (const SharedMMTexture &x : thing) {
                group_m.add(groupName, x);
            }

            group_m.validate(groupName);

            const int targetWidth = group_m.getWinner();
            const int targetHeight = targetWidth;

            group_m.log(groupName, targetWidth);

            auto &first = deref(thing.front()->get());
            const bool useImages = !group_m.has_file;
            const int manualMipLevels = group_m.max_manual_mip_levels;

            auto init2dTextureArray = [useImages,
                                       numLayers = static_cast<int>(thing.size()),
                                       targetWidth,
                                       targetHeight,
                                       manualMipLevels,
                                       &first](QOpenGLTexture &tex) {
                using Dir = QOpenGLTexture::CoordinateDirection;
                tex.setWrapMode(Dir::DirectionS, first.wrapMode(Dir::DirectionS));
                tex.setWrapMode(Dir::DirectionT, first.wrapMode(Dir::DirectionT));
                tex.setMinMagFilters(first.minificationFilter(), first.magnificationFilter());
                tex.setAutoMipMapGenerationEnabled(false);
                tex.create();
                tex.setSize(targetWidth, targetHeight, 1);
                tex.setLayers(numLayers);
                tex.setMipLevels(useImages ? manualMipLevels : tex.maximumMipLevels());
                tex.setFormat(first.format());
                tex.allocateStorage(QOpenGLTexture::PixelFormat::RGBA,
                                    QOpenGLTexture::PixelType::UInt8);
            };

            const bool forbidUpdates = useImages;
            pArrayTex = MMTexture::alloc(QOpenGLTexture::Target2DArray,
                                         init2dTextureArray,
                                         forbidUpdates);
            assert(pArrayTex->canBeUpdated() == !forbidUpdates);
            const auto id = assignId(pArrayTex);

            int pos = 0;
            for (const SharedMMTexture &x : thing) {
                if (useImages) {
                    auto images = x->getImages();
                    for (size_t level = 0; level < images.size(); ++level) {
                        const int tw = std::max(1, targetWidth >> level);
                        const int th = std::max(1, targetHeight >> level);
                        if (images[level].width() != tw || images[level].height() != th) {
                            throw std::runtime_error("image size wrong for level");
                        }
                        images[level] = images[level].convertToFormat(QImage::Format_RGBA8888);
                    }
                    opengl.uploadArrayLayer(pArrayTex, pos, images);
                } else {
                    QImage image = QImage{x->getName()}.mirrored().convertToFormat(
                        QImage::Format_RGBA8888);
                    if (image.width() != targetWidth || image.height() != targetHeight) {
                        MMLOG_WARNING()
                            << "[initTextures] Group '" << groupName << "': Image '"
                            << mmqt::toStdStringUtf8(x->getName()) << "' has dimensions "
                            << image.width() << "x" << image.height() << ", but the array expects "
                            << targetWidth << "x" << targetHeight << ". Resizing.";

                        image = image.scaled(targetWidth,
                                             targetHeight,
                                             Qt::IgnoreAspectRatio,
                                             Qt::SmoothTransformation);
                    }
                    opengl.uploadArrayLayer(pArrayTex, pos, {image});
                }
                x->setArrayPosition(MMTexArrayPosition{id, pos});
                pos += 1;
            }

            if (!useImages) {
                opengl.generateMipmaps(pArrayTex);
            }
        };

        auto initGroup = [&](const std::string_view groupName, auto &&...sources) {
            SharedMMTexture pArrayTex;
            auto thing = combine(std::forward<decltype(sources)>(sources)...);
            maybeCreateArray2(groupName, thing, pArrayTex);
            return pArrayTex;
        };

        textures.load_Array = textures.mob_Array = textures.no_ride_Array
            = initGroup("load+mob+no_ride", textures.load, textures.mob, textures.no_ride);

        textures.terrain_Array = textures.road_Array = initGroup("terrain+road",
                                                                 textures.terrain,
                                                                 textures.road);

        textures.white_pixel_Array = initGroup("white_pixel", textures.white_pixel);

        textures.exit_climb_down_Array = textures.exit_climb_up_Array = textures.exit_down_Array
            = textures.exit_up_Array = initGroup("exits",
                                                 textures.exit_climb_down,
                                                 textures.exit_climb_up,
                                                 textures.exit_down,
                                                 textures.exit_up);

        auto maybeCreateArray =
            [&](const std::string_view groupName, auto &thing, SharedMMTexture &pArrayTex) {
                if (pArrayTex)
                    return;
                pArrayTex = initGroup(groupName, thing);
            };

#define XTEX(_TYPE, _NAME) maybeCreateArray(#_NAME, textures._NAME, textures._NAME##_Array);
        XFOREACH_MAPCANVAS_TEXTURES(XTEX)
#undef XTEX

        {
            auto &&os = MMLOG();
            os << "[initTextures] Global measurement summary:\n";

#define PRINT(x) \
    do { \
        os << "  global_m." #x " = " << (global_m.x) << "\n"; \
    } while (false)

            PRINT(max_input_w);
            PRINT(max_input_h);
            PRINT(total_layers);
            PRINT(max_manual_mip_levels);

#undef PRINT

            os << " Global size distribution:\n";
            for (auto const &[size, count] : global_m.counts) {
                os << "  - " << size << "x" << size << ": " << count << " image(s)\n";
            }
        }
    }

    if constexpr (IS_DEBUG_BUILD) {
        auto &&os = MMLOG();

        auto report = [&os](std::string_view what, const SharedMMTexture &tex) {
            os << what << " is " << tex->getId().value();
            if (tex->hasArrayPosition()) {
                const MMTexArrayPosition &pos = tex->getArrayPosition();
                os << " and is in " << pos.array.value();
                os << " at " << pos.position;
            }
            os << "\n";
        };

        auto wat = [&report](std::string_view what, const auto &thing) {
            if constexpr (std::is_same_v<std::decay_t<decltype(thing)>, SharedMMTexture>) {
                report(what, thing);
            } else
                for (const SharedMMTexture &tex : thing) {
                    report(what, tex);
                }
        };

#define XTEX(_TYPE, _NAME) wat(#_NAME, m_textures._NAME);
        XFOREACH_MAPCANVAS_TEXTURES(XTEX)
#undef XTEX
    }

    {
        // calling updateTextures() here depends on current multisampling status being reset.
        updateTextures();
    }

    textures.for_each([](SharedMMTexture &tex) { tex->clearSourceData(); });
}

namespace mctp {

namespace detail {

NODISCARD static MMTexArrayPosition copy_proxy(const SharedMMTexture &pTex)
{
    if (!pTex) {
        return MMTexArrayPosition{};
    }

    MMTexture &tex = *pTex;
    auto id = tex.getId();

    if ((false)) {
        MAYBE_UNUSED QOpenGLTexture *qtex = tex.get();
        QOpenGLTexture::Target target = tex.target();
        bool canBeUpdated = tex.canBeUpdated();

        qInfo() << "target:" << target << "/ id:" << id.value()
                << "/ canBeUpdated:" << canBeUpdated;
    }

    return tex.getArrayPosition();
}

template<typename T>
auto copy_proxy(const T &input) -> ::mctp::detail::proxy_t<T>
{
    using Output = ::mctp::detail::proxy_t<T>;
    using E = typename Output::index_type;
    constexpr size_t SIZE = Output::SIZE;
    Output output;
    for (size_t i = 0; i < SIZE; ++i) {
        const auto e = static_cast<E>(i);
        output[e] = copy_proxy(input[e]);
    }
    return output;
}

} // namespace detail

MapCanvasTexturesProxy getProxy(const MapCanvasTextures &mct)
{
    MapCanvasTexturesProxy result;
#define X_COPY_PROXY(_Type, _Name) result._Name = detail::copy_proxy(mct._Name);
    XFOREACH_MAPCANVAS_TEXTURES(X_COPY_PROXY)
#undef X_COPY_PROXY
    return result;
}
} // namespace mctp

void MapCanvas::updateTextures()
{
    const bool wantTrilinear = getConfig().canvas.trilinearFiltering.get();
    m_textures.for_each(
        [wantTrilinear](SharedMMTexture &tex) -> void { ::setTrilinear(tex, wantTrilinear); });
    m_textures.for_each_array(
        [wantTrilinear](std::string_view /*array_name*/, const SharedMMTexture &arr) {
            ::setTrilinear(arr, wantTrilinear);
        });

    // called to trigger an early error
    std::ignore = mctp::getProxy(m_textures);
}
