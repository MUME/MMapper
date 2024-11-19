// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../configuration/NamedConfig.h"
#include "../configuration/configuration.h"
#include "../display/MapCanvasConfig.h"
#include "../display/MapCanvasData.h"
#include "../display/mapcanvas.h"
#include "../global/Consts.h"
#include "../global/NamedColors.h"
#include "../global/PrintUtils.h"
#include "../proxy/proxy.h"
#include "../syntax/SyntaxArgs.h"
#include "../syntax/TreeParser.h"
#include "AbstractParser-Utils.h"
#include "abstractparser.h"

#include <ostream>

#include <QColor>

class NODISCARD ArgNamedColor final : public syntax::IArgument
{
private:
    NODISCARD syntax::MatchResult virt_match(const syntax::ParserInput &input,
                                             syntax::IMatchErrorLogger *) const override;

    std::ostream &virt_to_stream(std::ostream &os) const override;
};

syntax::MatchResult ArgNamedColor::virt_match(const syntax::ParserInput &input,
                                              syntax::IMatchErrorLogger *) const
{
    if (input.empty())
        return syntax::MatchResult::failure(input);

    auto arg = std::string_view{input.front()};

    auto names = XNamedColor::getAllNames();
    for (const auto &name : names) {
        if (name == arg)
            return syntax::MatchResult::success(1, input, Value{name});
    }

    return syntax::MatchResult::failure(input);
}

std::ostream &ArgNamedColor::virt_to_stream(std::ostream &os) const
{
    return os << "<NamedColor>";
}

class NODISCARD ArgHexColor final : public syntax::IArgument
{
private:
    NODISCARD syntax::MatchResult virt_match(const syntax::ParserInput &input,
                                             syntax::IMatchErrorLogger *) const override;

    std::ostream &virt_to_stream(std::ostream &os) const override;
};

syntax::MatchResult ArgHexColor::virt_match(const syntax::ParserInput &input,
                                            syntax::IMatchErrorLogger *) const
{
    if (input.empty())
        return syntax::MatchResult::failure(input);

    auto arg = StringView{input.front()};
    if (!arg.startsWith("#"))
        return syntax::MatchResult::failure(input);
    ++arg;

    for (const char c : arg) {
        if (!std::isxdigit(c))
            return syntax::MatchResult::failure(input);
    }

    if (arg.length() != 6) {
        return syntax::MatchResult::failure(input);
    }

    const auto color = Color::fromHex(arg.getStdStringView());
    return syntax::MatchResult::success(1, input, Value{static_cast<int64_t>(color.getRGB())});
}

std::ostream &ArgHexColor::virt_to_stream(std::ostream &os) const
{
    return os << "<HexColor>";
}

class NODISCARD BoolAlpha final
{
private:
    bool m_value = false;

public:
    explicit BoolAlpha(const bool value)
        : m_value{value}
    {}

    friend std::ostream &operator<<(std::ostream &os, const BoolAlpha &x)
    {
        return os << (x.m_value ? "true" : "false");
    }
};

template<typename T>
inline decltype(auto) remap(T &&x)
{
    static_assert(!std::is_same_v<std::decay_t<T>, std::string_view>);
    if constexpr (std::is_same_v<std::decay_t<T>,
                                 std::string> || std::is_same_v<std::decay_t<T>, const char *>)
        return syntax::abbrevToken(std::forward<T>(x));
    else if constexpr (true)
        return std::forward<T>(x);

    std::abort();
}

// REVISIT: Can we (ab)use tuple to remap std::string to syntax::abbrevToken?
template<typename... Args>
NODISCARD static auto syn(Args &&...args)
{
    return syntax::buildSyntax(remap(std::forward<Args>(args))...);
}

void AbstractParser::doConfig(const StringView cmd)
{
    auto listColors = syntax::Accept(
        [](User &user, const Pair *) {
            auto &os = user.getOstream();

            auto names = XNamedColor::getAllNames();
            std::sort(names.begin(), names.end());

            os << "Customizable colors:" << std::endl;
            for (const auto &name : names) {
                if (name.empty() || name[0] == char_consts::C_PERIOD)
                    continue;
                XNamedColor color(name);
                os << " " << SmartQuotedString{name} << " = " << color.getColor() << std::endl;
            }
        },
        "list colors");

    auto setNamedColor = syntax::Accept(
        [this](User &user, const Pair *args) {
            auto &os = user.getOstream();
            if (args == nullptr || args->cdr == nullptr || !args->car.isLong()
                || !args->cdr->car.isString()) {
                throw std::runtime_error("internal error");
            }

            const std::string name = args->cdr->car.getString();
            const auto rgb = static_cast<uint32_t>(args->car.getLong());

            auto color = XNamedColor{name};
            const auto oldColor = color.getColor();
            const auto newColor = Color::fromRGB(rgb);

            if (oldColor.getRGB() == rgb) {
                os << "Color " << SmartQuotedString{name} << " is already " << color.getColor()
                   << "." << std::endl;
                return;
            }

            color.setColor(newColor);
            os << "Color " << SmartQuotedString{name} << " has been changed from " << oldColor
               << " to " << color.getColor() << "." << std::endl;

            // FIXME: Some of the colors still require a map update.
            if ((false))
                graphicsSettingsChanged();
            else
                mapChanged();
        },
        "set named color");

    auto makeSetFixedPoint = [this](FixedPoint<1> &fp, const std::string &help) -> syntax::Accept {
        //
        return syntax::Accept(
            [this, &fp, help](User &user, const Pair *const args) -> void {
                auto &os = user.getOstream();

                if (args == nullptr || !args->car.isFloat())
                    throw std::runtime_error("internal type error");

                const float value = args->car.getFloat();
                const auto min = fp.clone(fp.min).getFloat();
                const auto max = fp.clone(fp.max).getFloat();
                if (value < min || value > max) {
                    throw std::runtime_error("internal bounds error");
                }

                const int oldValue = fp.get();
                auto clone = fp.clone(oldValue);
                clone.setFloat(value);
                if (clone.get() == oldValue) {
                    os << "No change: " << help << " is already " << fp.getFloat() << std::endl;
                    return;
                }

                clone.set(oldValue);
                fp.setFloat(value);
                os << "Changed " << help << " from " << clone.getFloat() << " to " << fp.getFloat()
                   << std::endl;
                this->graphicsSettingsChanged();
            },
            "set " + help);
    };

    using namespace syntax;

    const auto argBool = TokenMatcher::alloc<ArgBool>();
    const auto argInt = TokenMatcher::alloc<ArgInt>();
    const auto optArgEquals = TokenMatcher::alloc<ArgOptionalChar>(char_consts::C_EQUALS);

    auto makeFixedPointArg = [optArgEquals,
                              &makeSetFixedPoint](FixedPoint<1> &fp,
                                                  const std::string &help) -> SharedConstSublist {
        const auto min = fp.clone(fp.min).getFloat();
        const auto max = fp.clone(fp.max).getFloat();
        return syn(help,
                   optArgEquals,
                   TokenMatcher::alloc_copy<ArgFloat>(ArgFloat::withMinMax(min, max)),
                   makeSetFixedPoint(fp, help));
    };

    auto &advanced = setConfig().canvas.advanced;

    auto getZoom = []() -> float {
        if (auto primary = MapCanvas::getPrimary())
            return primary->getRawZoom();
        return 1.f;
    };

    auto setZoom = [this](float f) -> bool {
        if (auto primary = MapCanvas::getPrimary()) {
            primary->setZoom(f);
            this->graphicsSettingsChanged();
            return true;
        }

        return false;
    };

    auto zoomSyntax = [&getZoom, &setZoom]() -> SharedConstSublist {
        auto argZoom = TokenMatcher::alloc_copy<ArgFloat>(
            ArgFloat::withMinMax(ScaleFactor::MIN_VALUE, ScaleFactor::MAX_VALUE));
        auto acceptZoom = Accept(
            [&getZoom, &setZoom](User &user, const Pair *const args) -> void {
                auto &os = user.getOstream();

                if (args == nullptr || !args->car.isFloat())
                    throw std::runtime_error("internal type error");

                const float value = args->car.getFloat();
                const auto min = ScaleFactor::MIN_VALUE;
                const auto max = ScaleFactor::MAX_VALUE;
                if (value < min || value > max) {
                    throw std::runtime_error("internal bounds error");
                }

                const float oldValue = getZoom();
                if (utils::equals(value, oldValue)) {
                    os << "No change: zoom is already " << oldValue << std::endl;
                    return;
                }

                if (setZoom(value))
                    os << "Changed zoom from " << oldValue << " to " << value << std::endl;
                else
                    os << "Unable to change zoom." << std::endl;
            },
            "set zoom");
        return syn("zoom", syn("set", argZoom, acceptZoom));
    }();

    auto opt = [this, argBool, optArgEquals](const char *const name,
                                             NamedConfig<bool> &conf,
                                             std::string help) {
        return syn(name,
                   optArgEquals,
                   argBool,
                   Accept(
                       [this, &conf](User &user, const Pair *const args) {
                           const auto value = deref(args).car.getBool();
                           auto &os = user.getOstream();

                           if (conf.get() == value) {
                               os << conf.getName() << " is already " << BoolAlpha(value)
                                  << std::endl;
                               return;
                           }

                           conf.set(value);
                           os << "Set " << conf.getName() << " = " << BoolAlpha(value) << std::endl;
                           graphicsSettingsChanged();
                       },
                       std::move(help)));
    };

    const auto configSyntax = syn(
        syn("mode",
            syn("play",
                Accept(
                    [this](User &user, auto) {
                        setMode(MapModeEnum::PLAY);
                        send_ok(user.getOstream());
                    },
                    "play mode")),
            syn("mapping",
                Accept(
                    [this](User &user, auto) {
                        setMode(MapModeEnum::MAP);
                        send_ok(user.getOstream());
                    },
                    "mapping mode")),
            syn("emulation",
                Accept(
                    [this](User &user, auto) {
                        setMode(MapModeEnum::OFFLINE);
                        send_ok(user.getOstream());
                    },
                    "offline emulation mode"))),
        syn("file",
            // TODO: add a command to show what's different from the factory default values,
            // and another command to show what's different from the current save file,
            // or just a list of {key, default, saved, current}?
            syn("save",
                Accept(
                    [](User &user, auto) {
                        auto &os = user.getOstream();
                        os << "Saving config file..." << std::endl;
                        getConfig().write();
                        os << "Saved." << std::endl;
                    },
                    "save config file")),
            syn("load",
                Accept(
                    [this](User &user, auto) {
                        auto &os = user.getOstream();
                        if (m_proxy.isConnected()) {
                            os << "You must disconnect before you can reload the saved configuration."
                               << std::endl;
                            return;
                        }
                        os << "Loading saved file..." << std::endl;
                        setConfig().read();
                        send_ok(os);
                    },
                    "read config file")),
            syn("factory",
                "reset",
                TokenMatcher::alloc<ArgStringExact>("Yes, I'm sure!"),
                Accept(
                    [this](User &user, auto) {
                        auto &os = user.getOstream();
                        if (m_proxy.isConnected()) {
                            os << "You must disconnect before you can do a factory reset."
                               << std::endl;
                            return;
                        }
                        // REVISIT: only allow this when you're disconnected?
                        os << "Performing factory reset..." << std::endl;
                        setConfig().reset();
                        os << "WARNING: You have just reset your configuration." << std::endl;
                    },
                    "factory reset the config"))),
        syn("map",
            syn("colors",
                syn(syn("list", listColors),
                    syn("set",
                        TokenMatcher::alloc<ArgNamedColor>(),
                        optArgEquals,
                        TokenMatcher::alloc<ArgHexColor>(),
                        setNamedColor))),
            syn("perf-stats",
                syn("set", opt("enabled", advanced.printPerfStats, "enable/disable stats"))),
            zoomSyntax,
            syn("3d-camera",
                syn("set",
                    opt("enabled", advanced.use3D, "enable/disable 3d camera"),
                    opt("auto-tilt", advanced.autoTilt, "enable/disable 3d auto tilt"),
                    makeFixedPointArg(advanced.fov, "fov"),
                    makeFixedPointArg(advanced.verticalAngle, "pitch"),
                    makeFixedPointArg(advanced.horizontalAngle, "yaw"),
                    makeFixedPointArg(advanced.layerHeight, "layer-height")))));

    eval("config", configSyntax, cmd);
}

void AbstractParser::setMode(MapModeEnum mode)
{
    emit sig_setMode(mode);
}
