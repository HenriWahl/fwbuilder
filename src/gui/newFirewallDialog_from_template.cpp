/*

                          Firewall Builder

                 Copyright (C) 2003 NetCitadel, LLC

  Author:  Vadim Kurland     vadim@fwbuilder.org

  $Id$

  This program is free software which we release under the GNU General Public
  License. You may redistribute and/or modify this program under the terms
  of that license as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  To get a copy of the GNU General Public License, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include "../../config.h"
#include "global.h"
#include "platforms.h"

#include "newFirewallDialog.h"
#include "FWBSettings.h"
#include "FWBTree.h"
#include "InterfaceEditorWidget.h"
#include "InterfacesTabWidget.h"

#include "fwbuilder/Library.h"
#include "fwbuilder/Firewall.h"
#include "fwbuilder/Resources.h"
#include "fwbuilder/Policy.h"
#include "fwbuilder/IPv4.h"
#include "fwbuilder/IPv6.h"

#include <QtDebug>
#include <QMessageBox>

using namespace libfwbuilder;
using namespace std;


void newFirewallDialog::createFirewallFromTemplate()
{
    QListWidgetItem *itm = m_dialog->templateList->currentItem();
    FWObject *template_fw=templates[itm];
    assert (template_fw!=NULL);

    string platform = readPlatform(m_dialog->platform).toAscii().constData();
    string host_os = readHostOS(m_dialog->hostOS).toAscii().constData();


    FWObject *no;
    no = db->create(Firewall::TYPENAME);

    if (no==NULL)
    {
        QDialog::accept();
        return;
    }

    parent->add(no);
    no->duplicate(template_fw, true);
    no->setName(m_dialog->obj_name->text().toStdString());

    nfw = Firewall::cast(no);

    no->setStr("platform", platform);
    Resources::setDefaultTargetOptions(platform , nfw);
    no->setStr("host_OS", host_os);
    Resources::setDefaultTargetOptions(host_os , nfw);
}

void newFirewallDialog::changedAddressesInNewFirewall()
{
    // the key in this map is the pointer to interface that used to be
    // part of the template. We do not allow the user to create new or 
    // delete existing interfaces when they edit template interfaces.

    QMap<Interface*, EditedInterfaceData> new_configuration =
        m_dialog->interfaceEditor2->getData();

    list<FWObject*> all_intrfaces = nfw->getByTypeDeep(Interface::TYPENAME);
    for (list<FWObject*>::iterator intiter=all_intrfaces.begin();
         intiter != all_intrfaces.end(); ++intiter )
    {
        Interface *intf = Interface::cast(*intiter);

        list<InetAddrMask> old_subnets;

        list<FWObject*> old_addr = intf->getByType(IPv4::TYPENAME);
        list<FWObject*> old_ipv6 = intf->getByType(IPv6::TYPENAME);
        old_addr.splice(old_addr.begin(), old_ipv6);

        while (old_addr.size())
        {
            Address *addr = Address::cast(old_addr.front());
            old_addr.pop_front();
            if (addr)
            {
                const InetAddrMask *old_addr_mask = addr->getInetAddrMaskObjectPtr();
                InetAddrMask old_net = InetAddrMask(
                    *(old_addr_mask->getAddressPtr()),
                    *(old_addr_mask->getNetmaskPtr()));
                old_subnets.push_back(old_net);
                intf->remove(addr, false);
                delete addr;
            }
        }

        if (new_configuration.count(intf) == 0)
        {
            QMessageBox::critical(
                this, "Firewall Builder",
                tr("Can not find interface %1 in the interface editor data")
                .arg(intf->getName().c_str()),
                "&Continue", QString::null, QString::null, 0, 1 );
        } else
        {
            EditedInterfaceData new_data = new_configuration[intf];
            intf->setName(new_data.name.toStdString());
            intf->setLabel(new_data.label.toStdString());
            intf->setComment(new_data.comment.toStdString());

            if (fwbdebug)
                qDebug() << "Interface" << intf->getName().c_str()
                         << "type=" << new_data.type;


            switch (new_data.type)
            {
            case 1:
                intf->setDyn(true);
                intf->setUnnumbered(false);
                break;
            case 2:
                intf->setDyn(false);
                intf->setUnnumbered(true);
                break;
            default:
                intf->setDyn(false);
                intf->setUnnumbered(false);
                break;
            }

            QMap<libfwbuilder::Address*, AddressInfo >::iterator addrit;
            for (addrit=new_data.addresses.begin(); addrit!=new_data.addresses.end(); ++addrit)
            {
                AddressInfo new_addr = addrit.value();
                if (new_addr.ipv4)
                {
                    IPv4 *oa = IPv4::cast(db->create(IPv4::TYPENAME));
                    intf->add(oa);
                    QString name = QString("%1:%2:ipv4")
                        .arg(nfw->getName().c_str())
                        .arg(intf->getName().c_str());
                    oa->setName(name.toStdString());
                    oa->setAddress( InetAddr(new_addr.address.toLatin1().constData()) );
                    bool ok = false ;
                    int inetmask = new_addr.netmask.toInt(&ok);
                    if (ok)
                    {
                        oa->setNetmask( InetAddr(inetmask) );
                    }
                    else
                    {
                        oa->setNetmask(
                            InetAddr(new_addr.netmask.toLatin1().constData()) );
                    }

                } else
                {

                }
            }
        }
    }
}

