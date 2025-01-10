// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "GroupPortMapper.h"

#include <memory>

#include <QDebug>

#ifndef MMAPPER_NO_MINIUPNPC
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#endif

struct NODISCARD GroupPortMapper::Pimpl
{
    Pimpl() = default;
    virtual ~Pimpl();

private:
    NODISCARD virtual QByteArray virt_tryGetExternalIp() = 0;
    NODISCARD virtual bool virt_tryAddPortMapping(uint16_t port) = 0;
    NODISCARD virtual bool virt_tryDeletePortMapping(uint16_t port) = 0;

public:
    NODISCARD QByteArray tryGetExternalIp() { return virt_tryGetExternalIp(); }
    NODISCARD bool tryAddPortMapping(uint16_t port) { return virt_tryAddPortMapping(port); }
    NODISCARD bool tryDeletePortMapping(uint16_t port) { return virt_tryDeletePortMapping(port); }
};

GroupPortMapper::Pimpl::~Pimpl() = default;

class NODISCARD NoopPortMapper final : public GroupPortMapper::Pimpl
{
public:
    NoopPortMapper() = default;
    ~NoopPortMapper() final;

private:
    NODISCARD QByteArray virt_tryGetExternalIp() final
    {
        // TODO: Use a 3rd party service like checkip.dyndns.org
        return "";
    }
    NODISCARD bool virt_tryAddPortMapping(const uint16_t) final { return false; }
    NODISCARD bool virt_tryDeletePortMapping(const uint16_t) final { return false; }
};

NoopPortMapper::~NoopPortMapper() = default;

#ifndef MMAPPER_NO_MINIUPNPC
static constexpr const auto MM_UPNP_DESCRIPTION = "MMapper";
static constexpr const auto MM_UPNP_WHITELISTED_PROTO = "TCP";
static constexpr const auto MM_UPNP_PERMANENT_LEASE = "0";
using UPNPDev_ptr = std::unique_ptr<UPNPDev, decltype(&::freeUPNPDevlist)>;

#if MINIUPNPC_API_VERSION < 19
#define MM_UPNP_NO_IGD (0)
#define MM_UPNP_CONNECTED_IGD (1)
#define MM_UPNP_PRIVATEIP_IGD (2)
#define MM_UPNP_DISCONNECTED_IGD (3)
#define MM_UPNP_UNKNOWN_DEVICE (4)
#else
#define MM_UPNP_NO_IGD UPNP_NO_IGD
#define MM_UPNP_CONNECTED_IGD UPNP_CONNECTED_IGD
#define MM_UPNP_PRIVATEIP_IGD UPNP_PRIVATEIP_IGD
#define MM_UPNP_DISCONNECTED_IGD UPNP_DISCONNECTED_IGD
#define MM_UPNP_UNKNOWN_DEVICE UPNP_UNKNOWN_DEVICE
#endif

class NODISCARD MiniUPnPcPortMapper final : public GroupPortMapper::Pimpl
{
private:
    UPNPDev_ptr deviceList{nullptr, ::freeUPNPDevlist};
    UPNPUrls urls{};
    IGDdatas igdData{};
    char lanAddress[64]{};
    char wanAddress[64]{};
    int validIGDState = 0;

public:
    MiniUPnPcPortMapper()
    {
        int result = UPNPDISCOVER_SUCCESS;
        deviceList = UPNPDev_ptr
        {
            upnpDiscover(1000,
                         nullptr,
                         nullptr,
                         0,
                         0,
#if MINIUPNPC_API_VERSION >= 14
                         2,
#endif
                         &result),
                ::freeUPNPDevlist
        };

        if (result != UPNPDISCOVER_SUCCESS) {
            // TODO: handle this error.
        }

        validIGDState = UPNP_GetValidIGD(deviceList.get(),
                                         &urls,
                                         &igdData,
                                         lanAddress,
                                         sizeof lanAddress
#if MINIUPNPC_API_VERSION >= 18
                                         ,
                                         wanAddress,
                                         sizeof wanAddress
#endif
        );
        switch (validIGDState) {
        case MM_UPNP_NO_IGD:
            qInfo() << "No IGD found";
            break;
        case MM_UPNP_CONNECTED_IGD:
            qInfo() << "Valid IGD found";
            break;
        case MM_UPNP_PRIVATEIP_IGD:
            qInfo() << "Valid IGD has been found but it reported as not connected";
            break;
        case MM_UPNP_DISCONNECTED_IGD:
            qInfo() << "UPnP device has been found but was not recognized as an IGD";
            break;
        case MM_UPNP_UNKNOWN_DEVICE:
        default:
            qWarning() << "UPNP_GetValidIGD returned an unknown result code" << validIGDState;
            break;
        }
    }
    ~MiniUPnPcPortMapper() final;

    NODISCARD bool validIGD() const
    {
        return validIGDState == MM_UPNP_CONNECTED_IGD;
    }

private:
    NODISCARD QByteArray virt_tryGetExternalIp() final
    {
        if (!validIGD()) {
            return "";
        }

        // REVISIT: Expose the external IP in the preferences?
        static const constexpr int EXTERNAL_IP_ADDRESS_BYTES = 46; /* ipv6 requires 45 bytes */
        char externalAddress[EXTERNAL_IP_ADDRESS_BYTES]{};
        int result = UPNP_GetExternalIPAddress(urls.controlURL,
                                               igdData.first.servicetype,
                                               externalAddress);
        if (result != UPNPCOMMAND_SUCCESS) {
            qWarning() << "UPNP_GetExternalIPAddress returned" << result;
            return "";
        }

        if (!externalAddress[0]) {
            qWarning() << "IGD unable to retrieve external IP";
            return "";
        }

        qDebug() << "IGD reported external IP" << externalAddress;
        return QByteArray(externalAddress, EXTERNAL_IP_ADDRESS_BYTES);
    }

    NODISCARD bool virt_tryAddPortMapping(const uint16_t port) final
    {
        if (!validIGD()) {
            qDebug() << "No IGD found to add a port mapping to";
            return false;
        }

        const auto portString = QString("%1").arg(port).toLocal8Bit();
        int result = UPNP_AddPortMapping(urls.controlURL,
                                         igdData.first.servicetype,
                                         portString.constData(),
                                         portString.constData(),
                                         lanAddress,
                                         MM_UPNP_DESCRIPTION,
                                         MM_UPNP_WHITELISTED_PROTO,
                                         nullptr,
                                         MM_UPNP_PERMANENT_LEASE);
        if (result != UPNPCOMMAND_SUCCESS) {
            qWarning() << "UPNP_AddPortMapping failed with result code" << result;
            return false;
        }

        qDebug() << "Added IGD port mapping";
        return true;
    }

    NODISCARD bool virt_tryDeletePortMapping(const uint16_t port) final
    {
        if (!validIGD()) {
            qDebug() << "No IGD found to remove a port mapping from";
            return false;
        }

        const auto portString = QString("%1").arg(port).toLocal8Bit();
        int result = UPNP_DeletePortMapping(urls.controlURL,
                                            igdData.first.servicetype,
                                            portString.constData(),
                                            MM_UPNP_WHITELISTED_PROTO,
                                            nullptr);
        if (result != UPNPCOMMAND_SUCCESS) {
            qWarning() << "UPNP_DeletePortMapping failed with result code" << result;
            return false;
        }

        qDebug() << "Deleted IGD port mapping";
        return true;
    }
};

MiniUPnPcPortMapper::~MiniUPnPcPortMapper()
{
    FreeUPNPUrls(&urls);
}

GroupPortMapper::GroupPortMapper()
    : m_pimpl{std::make_unique<MiniUPnPcPortMapper>()}
{}
#else
GroupPortMapper::GroupPortMapper()
    : m_pimpl{std::make_unique<NoopPortMapper>()}
{}
#endif

GroupPortMapper::~GroupPortMapper() = default;

QByteArray GroupPortMapper::tryGetExternalIp()
{
    return m_pimpl->tryGetExternalIp();
}

bool GroupPortMapper::tryAddPortMapping(const uint16_t port)
{
    return m_pimpl->tryAddPortMapping(port);
}

bool GroupPortMapper::tryDeletePortMapping(const uint16_t port)
{
    return m_pimpl->tryDeletePortMapping(port);
}
