//
// kernel.h
//
// MiniDexed - Dexed FM synthesizer for bare metal Raspberry Pi
// Copyright (C) 2022  The MiniDexed Team
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef _kernel_h
#define _kernel_h

#include "circle_stdlib_app.h"
#include <circle/cputhrottle.h>
#include <circle/gpiomanager.h>
#include <circle/i2cmaster.h>
#include <circle/spimaster.h>
#include <display/hd44780device.h>
#include <display/ssd1306device.h>
#include <display/st7789device.h>
#include <display/font6x8.h>
#include <circle/types.h>
#include <circle/usb/usbcontroller.h>
#include <fatfs/ff.h>
#include <Properties/propertiesfatfsfile.h>
#include <circle/sysconfig.h>
#include <string>
//#include "config.h"
//#include "minijv880.h"

#define SPI_INACTIVE	255
#define SPI_DEF_CLOCK	15000	// kHz
#define SPI_DEF_MODE	0		// Default mode (0,1,2,3)

enum TShutdownMode
{
	ShutdownNone,
	ShutdownHalt,
	ShutdownReboot
};

class CKernel : public CStdlibAppStdio
{
public:
	CKernel (void);
	~CKernel (void);

	bool Initialize (void);

	TShutdownMode Run (void);
	bool InitDisplay (void);
	void UpdateDisplay (void);
	void LCDWrite (const char *pString);

private:
	static void PanicHandler (void);

private:
    CSSD1306Device* m_pSSD1306 = nullptr;
    CST7789Device* m_pST7789 = nullptr;
    CST7789Display* m_pST7789Display = nullptr;
    CHD44780Device* m_pHD44780 = nullptr;
	// do not change this order
	//CConfig		m_Config;
	CGPIOManager	m_GPIOManager;
	CI2CMaster	m_I2CMaster;
	CSPIMaster	*m_pSPIMaster;
	CCharDevice	*m_LCD;
	CWriteBufferDevice *m_pLCDBuffered;
	CCPUThrottle	m_CPUThrottle;
	CUSBController *m_pUSB;
	CPropertiesFatFsFile* m_pMiniDexedConfig; 
    CPropertiesFatFsFile* m_pConfig;

	//void m_pLCDBuffered(void);

	static CKernel *s_pThis;
};

#endif
