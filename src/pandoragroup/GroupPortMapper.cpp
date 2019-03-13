/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "GroupPortMapper.h"

#include <QDebug>

#ifndef MMAPPER_NO_MINIUPNPC
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#endif

struct GroupPortMapper::Pimpl
{
    explicit Pimpl() = default;
    virtual ~Pimpl();
    virtual QByteArray tryGetExternalIp() = 0;
    virtual bool tryAddPortMapping(const quint16 port) = 0;
    virtual bool tryDeletePortMapping(const quint16 port) = 0;
};

GroupPortMapper::Pimpl::~Pimpl() = default;

class NoopPortMapper final : public GroupPortMapper::Pimpl
{
public:
    explicit NoopPortMapper() = default;
    virtual ~NoopPortMapper() override;

    QByteArray tryGetExternalIp() override
    {
        // TODO: Use a 3rd party service like checkip.dyndns.org
        return "";
    }
    bool tryAddPortMapping(const quint16) override { return false; }
    bool tryDeletePortMapping(const quint16) override { return false; }
};

NoopPortMapper::~NoopPortMapper() = default;

#ifndef MMAPPER_NO_MINIUPNPC
static constexpr const auto UPNP_DESCRIPTION = "MMapper";
static constexpr const auto UPNP_WHITELISTED_PROTO = "TCP";
static constexpr const auto UPNP_PERMANENT_LEASE = "0";
using UPNPDev_ptr = std::unique_ptr<UPNPDev, decltype(&::freeUPNPDevlist)>;

class MiniUPnPcPortMapper final : public GroupPortMapper::Pimpl
{
private:
    UPNPDev_ptr deviceList{nullptr, ::freeUPNPDevlist};
    UPNPUrls urls;
    IGDdatas igdData;
    char lanAddress[64];
    int validIGDState = 0;

public:
    explicit MiniUPnPcPortMapper()
    {
        int result;
#if MINIUPNPC_API_VERSION < 14
        deviceList = UPNPDev_ptr(upnpDiscover(1000, nullptr, nullptr, 0, 0, &result),
                                 ::freeUPNPDevlist);
#else
        deviceList = UPNPDev_ptr(upnpDiscover(1000, nullptr, nullptr, 0, 0, 2, &result),
                                 ::freeUPNPDevlist);
#endif
        validIGDState = UPNP_GetValidIGD(deviceList.get(),
                                         &urls,
                                         &igdData,
                                         lanAddress,
                                         sizeof lanAddress);
        if (validIGDState != 0) {
            if (validIGDState == 1) {
                qInfo() << "Valid IGD found";
            } else if (result == 2) {
                qInfo() << "Valid IGD has been found but it reported as not connected";
            } else if (result == 3) {
                qInfo() << "UPnP device has been found but was not recognized as an IGD";
            } else {
                qWarning() << "UPNP_GetValidIGD returned an unknown result code";
            }
        } else {
            qInfo() << "No IGD found";
        }
    }
    virtual ~MiniUPnPcPortMapper() override;

    QByteArray tryGetExternalIp() override
    {
        if (validIGDState == 1) {
            // REVISIT: Expose the external IP in the preferences?
            char externalAddress[16];
            if (UPNP_GetExternalIPAddress(urls.controlURL,
                                          igdData.first.servicetype,
                                          externalAddress)
                != UPNPCOMMAND_SUCCESS)
                externalAddress[0] = '\0';
            qDebug() << "IGD reported external IP" << externalAddress;
            return QByteArray::fromRawData(externalAddress, 16);
        }
        return QByteArray("");
    }

    bool tryAddPortMapping(const quint16 port) override
    {
        if (validIGDState == 1) {
            const auto portString = QString("%1").arg(port).toLocal8Bit();
            int result = UPNP_AddPortMapping(urls.controlURL,
                                             igdData.first.servicetype,
                                             portString.constData(),
                                             portString.constData(),
                                             lanAddress,
                                             UPNP_DESCRIPTION,
                                             UPNP_WHITELISTED_PROTO,
                                             nullptr,
                                             UPNP_PERMANENT_LEASE);
            if (result != UPNPCOMMAND_SUCCESS) {
                qWarning() << "UPNP_AddPortMapping failed with result code" << result;
            } else {
                qDebug() << "Added IGD port mapping";
                return true;
            }
        } else {
            qDebug() << "No IGD found to add a port mapping to";
        }
        return false;
    }

    bool tryDeletePortMapping(const quint16 port) override
    {
        if (validIGDState == 1) {
            const auto portString = QString("%1").arg(port).toLocal8Bit();
            int result = UPNP_DeletePortMapping(urls.controlURL,
                                                igdData.first.servicetype,
                                                portString.constData(),
                                                UPNP_WHITELISTED_PROTO,
                                                nullptr);
            if (result != UPNPCOMMAND_SUCCESS) {
                qWarning() << "UPNP_DeletePortMapping failed with result code" << result;
            } else {
                qDebug() << "Deleted IGD port mapping";
                return true;
            }
        } else {
            qDebug() << "No IGD found to remove a port mapping from";
        }
        return false;
    }
};

MiniUPnPcPortMapper::~MiniUPnPcPortMapper()
{
    if (validIGDState != 0)
        FreeUPNPUrls(&urls);
}

GroupPortMapper::GroupPortMapper()
    : m_pimpl{std::unique_ptr<Pimpl>(static_cast<Pimpl *>(new MiniUPnPcPortMapper()))}
{}
#else
GroupPortMapper::GroupPortMapper()
    : m_pimpl{std::unique_ptr<Pimpl>(static_cast<Pimpl *>(new NoopPortMapper()))}
{}
#endif

GroupPortMapper::~GroupPortMapper() = default;

QByteArray GroupPortMapper::tryGetExternalIp()
{
    return m_pimpl->tryGetExternalIp();
}

bool GroupPortMapper::tryAddPortMapping(const quint16 port)
{
    return m_pimpl->tryAddPortMapping(port);
}

bool GroupPortMapper::tryDeletePortMapping(const quint16 port)
{
    return m_pimpl->tryDeletePortMapping(port);
}
