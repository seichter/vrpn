/** @file
	@brief Header

	@date 2011

	@author
	Ryan Pavlik
	<rpavlik@iastate.edu> and <abiryan@ryand.net>
	http://academic.cleardefinition.com/
	Iowa State University Virtual Reality Applications Center
	Human-Computer Interaction Graduate Program
*/

//          Copyright Iowa State University 2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef INCLUDED_HIDDevice_h_GUID_15239204_33b6_4b81_80f3_315b52f17eef
#define INCLUDED_HIDDevice_h_GUID_15239204_33b6_4b81_80f3_315b52f17eef

// Internal Includes
#include "vrpn_Shared.h"

// Library/third-party includes
#include <QObject>
#include <QByteArray>

// Standard includes
// - none

class vrpn_HidAcceptor;

class HIDDevice: public QObject {
		Q_OBJECT
	public:
		explicit HIDDevice(vrpn_HidAcceptor * acceptor, QObject * parent = NULL);
		~HIDDevice();
	signals:
		void inputReport(QByteArray buffer);
	public slots:
		void do_update();

	protected:
		class VRPNDevice;
		friend class HIDDevice::VRPNDevice;
		void send_data_signal(size_t bytes, const char * buffer);
		VRPNDevice * _device;

};
#endif // INCLUDED_HIDDevice_h_GUID_15239204_33b6_4b81_80f3_315b52f17eef
