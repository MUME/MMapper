// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Textures.h"

#include "../configuration/configuration.h"
#include "../global/thread_utils.h"
#include "../global/utils.h"
#include "../opengl/OpenGLTypes.h"
#include "Filenames.h"
#include "RoadIndex.h"
#include "mapcanvas.h"

#include <array>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <vector>

#include <glm/glm.hpp>

#include <QMessageLogContext>

MMTextureId allocateTextureId()
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    static MMTextureId next{0};
    return next++;
}

MMTexture::MMTexture(Badge<MMTexture>, const QString &name)
    : m_qt_texture{QImage{name}.mirrored()}
    , m_name{name}
    , m_sourceData{std::make_unique<SourceData>()}
{
    auto &tex = m_qt_texture;
    tex.setWrapMode(QOpenGLTexture::WrapMode::MirroredRepeat);
    tex.setMinMagFilters(QOpenGLTexture::Filter::LinearMipMapLinear, QOpenGLTexture::Filter::Linear);
}

MMTexture::MMTexture(Badge<MMTexture>, std::vector<QImage> images)
    : m_qt_texture{std::invoke([&images] {
        if (images.empty()) {
            throw std::logic_error("cannot construct MMTexture from empty vector of images");
        }
        assert(!images.empty());
        return QOpenGLTexture{images.front()};
    })}
    , m_sourceData{std::make_unique<SourceData>(std::move(images))}
{
    const auto &front = m_sourceData->m_images.front();
    size_t level = 0;
    for (const auto &im : m_sourceData->m_images) {
        assert(im.width() == im.height());
        assert(im.width() == (front.width() >> level));
        assert(im.height() == (front.height() >> level));
        ++level;
    }

    auto &tex = m_qt_texture;
    tex.setWrapMode(QOpenGLTexture::WrapMode::MirroredRepeat);
    tex.setMinMagFilters(QOpenGLTexture::Filter::NearestMipMapNearest,
                         QOpenGLTexture::Filter::Nearest);
    tex.setFormat(QOpenGLTexture::TextureFormat::RGBA8_UNorm);
    tex.setAutoMipMapGenerationEnabled(false);
    const int numLevels = static_cast<int>(level);
    tex.setMipLevels(numLevels);
    tex.setMipMaxLevel(numLevels - 1);
}

void MapCanvasTextures::destroyAll()
{
    for_each([](SharedMMTexture &tex) -> void { tex.reset(); });

    auto &&os = MMLOG();

    if constexpr (IS_DEBUG_BUILD)
        os << "destroying...\n";
#define XTEX(_TYPE, _NAME) \
    do { \
        if (auto &tex = _NAME##_Array) { \
            if constexpr (IS_DEBUG_BUILD) { \
                const auto layers = deref(deref(tex).get()).layers(); \
                os << "... " #_NAME << "_Array w/ " << layers << " layer" \
                   << ((layers == 1) ? "" : "s") << "\n"; \
            } \
            tex.reset(); \
        } \
    } while (false);
    XFOREACH_MAPCANVAS_TEXTURES(XTEX)
#undef XTEX

    if constexpr (IS_DEBUG_BUILD)
        os << "Done\n";
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
static void appendAll(std::vector<SharedMMTexture> &v, Type &&things)
{
    for (const SharedMMTexture &t : things)
        v.emplace_back(t);
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

    {
        textures.for_each([&assignId](SharedMMTexture &pTex) {
            //
            MAYBE_UNUSED auto ignored = assignId(pTex);
        });
    }

    {
        // We're going to create a texture with Target2DArray.
        // Measure first.

        struct NODISCARD Measurements final
        {
            int max_xy_size = 0;
            int layer_count = 0;
            int max_mip_levels = 0;
        };

        const Measurements m = std::invoke([&textures]() -> Measurements {
            Measurements m2;
            textures.for_each([&m2](const SharedMMTexture &pTex) -> void {
                const auto &tex = deref(pTex);
                const QOpenGLTexture &qtex = deref(tex.get());
                assert(qtex.target() == QOpenGLTexture::Target2D);

                const int width = qtex.width();
                const int height = qtex.height();
                assert(width == height);

                m2.max_xy_size = std::max(m2.max_xy_size, std::max(width, height));
                m2.layer_count += 1;
                m2.max_mip_levels = std::max(m2.max_mip_levels, qtex.mipLevels());
            });
            return m2;
        });

        auto maybeCreateArray2 = [&assignId, &opengl](auto &thing, SharedMMTexture &pArrayTex) {
            std::optional<std::pair<int, int>> bounds;
            auto getBounds = [](const auto &x) {
                return std::make_pair<int, int>(x->get()->width(), x->get()->height());
            };
            QOpenGLTexture *pFirst = nullptr;

            for (const SharedMMTexture &x : thing) {
                if (!bounds) {
                    pFirst = x->get();
                    bounds = getBounds(x);
                    const auto size = bounds->first;
                    if (size != bounds->second)
                        throw std::runtime_error("image must be square");
                    if (!utils::isPowerOfTwo(static_cast<uint32_t>(size)))
                        throw std::runtime_error("image size must be a power of two");
                }

                if (bounds != getBounds(x)) {
                    throw std::runtime_error("oops");
                }
            }

            auto &first = deref(pFirst);
            std::vector<QString> fileInputs;
            std::vector<std::vector<QImage>> imageInputs;
            int maxWidth = 0;
            int maxHeight = 0;
            int maxMipLevel = 0;

            for (const auto &x : thing) {
                if (!x->getName().isEmpty()) {
                    const QString filename = x->getName();
                    fileInputs.emplace_back(filename);
                    const QImage image{filename};
                    maxWidth = std::max(maxWidth, image.width());
                    maxHeight = std::max(maxHeight, image.height());
                } else {
                    const auto &images = x->getImages();
                    imageInputs.emplace_back(images);
                    auto &front = images.front();
                    assert(front.width() == front.height());
                    maxWidth = std::max(maxWidth, front.width());
                    maxHeight = std::max(maxHeight, front.height());
                    maxMipLevel = std::max(maxMipLevel, static_cast<int>(images.size()));
                }
            }

            const bool useImages = fileInputs.empty();
            if (!useImages) {
                assert(imageInputs.empty());
            }
            const auto numLayers = useImages ? imageInputs.size() : fileInputs.size();
            if (useImages) {
                assert(first.mipLevels() == maxMipLevel);
            }

            auto init2dTextureArray =
                [useImages, numLayers, maxWidth, maxHeight, maxMipLevel, &first](
                    QOpenGLTexture &tex) {
                    using Dir = QOpenGLTexture::CoordinateDirection;
                    tex.setWrapMode(Dir::DirectionS, first.wrapMode(Dir::DirectionS));
                    tex.setWrapMode(Dir::DirectionT, first.wrapMode(Dir::DirectionT));
                    tex.setMinMagFilters(first.minificationFilter(), first.magnificationFilter());
                    tex.setAutoMipMapGenerationEnabled(false);
                    tex.create();
                    tex.setSize(maxWidth, maxHeight, 1);
                    tex.setLayers(static_cast<int>(numLayers));
                    tex.setMipLevels(useImages ? maxMipLevel : tex.maximumMipLevels());
                    tex.setFormat(first.format());
                    tex.allocateStorage(QOpenGLTexture::PixelFormat::RGBA,
                                        QOpenGLTexture::PixelType::UInt8);
                };

            {
                const bool forbidUpdates = useImages;
                pArrayTex = MMTexture::alloc(QOpenGLTexture::Target2DArray,
                                             init2dTextureArray,
                                             forbidUpdates);
                if (useImages) {
                    opengl.initArrayFromImages(pArrayTex, imageInputs);
                } else {
                    opengl.initArrayFromFiles(pArrayTex, fileInputs);
                }
                assert(pArrayTex->canBeUpdated() == !forbidUpdates);
            }
            const auto id = assignId(pArrayTex);

            int pos = 0;
            for (const SharedMMTexture &x : thing) {
                x->setArrayPosition(MMTexArrayPosition{id, pos});
                pos += 1;
            }
        };

        {
            using One = std::array<SharedMMTexture, 1>;
            One no_ride{textures.no_ride};
            SharedMMTexture pArrayTex;
            auto thing = combine(textures.load, textures.mob, no_ride);
            maybeCreateArray2(thing, pArrayTex);
            textures.load_Array = textures.mob_Array = textures.no_ride_Array = pArrayTex;
        }

        {
            SharedMMTexture pArrayTex;
            auto thing = combine(textures.terrain, textures.road);
            maybeCreateArray2(thing, pArrayTex);
            textures.terrain_Array = textures.road_Array = pArrayTex;
        }

        {
            using Four = std::array<SharedMMTexture, 4>;
            Four exits{textures.exit_climb_down,
                       textures.exit_climb_up,
                       textures.exit_down,
                       textures.exit_up};
            SharedMMTexture pArrayTex;
            maybeCreateArray2(exits, pArrayTex);
            textures.exit_climb_down_Array = textures.exit_climb_up_Array = textures.exit_down_Array
                = textures.exit_up_Array = pArrayTex;
        }

        auto maybeCreateArray = [&maybeCreateArray2](auto &thing, SharedMMTexture &pArrayTex) {
            if (pArrayTex)
                return;
            if constexpr (std::is_same_v<std::decay_t<decltype(thing)>, SharedMMTexture>) {
                using JustOne = std::array<SharedMMTexture, 1>;
                JustOne justOne{thing};
                maybeCreateArray2(justOne, pArrayTex);
            } else {
                maybeCreateArray2(thing, pArrayTex);
            }
        };

#define XTEX(_TYPE, _NAME) maybeCreateArray(textures._NAME, textures._NAME##_Array);
        XFOREACH_MAPCANVAS_TEXTURES(XTEX)
#undef XTEX

        {
            auto &&os = MMLOG();
            os << "[initTextures] measurements:\n";

#define PRINT(x) \
    do { \
        os << " " #x " = " << (x) << "\n"; \
    } while (false)

            PRINT(m.max_xy_size);
            PRINT(m.layer_count);
            PRINT(m.max_mip_levels);

#undef PRINT
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

#define XTEX(_TYPE, _NAME) \
    do { \
        const SharedMMTexture &arr = m_textures._NAME##_Array; \
        if (arr) { \
            ::setTrilinear(arr, wantTrilinear); \
        } \
    } while (false);
    XFOREACH_MAPCANVAS_TEXTURES(XTEX)
#undef XTEX

    // called to trigger an early error
    std::ignore = mctp::getProxy(m_textures);
}
