//
// kernel.cpp
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
#include "kernel.h"
#include <circle/logger.h>
#include <circle/synchronize.h>
#include <circle/gpiopin.h>
#include <assert.h>
#include <circle/usb/usbhcidevice.h>

LOGMODULE ("kernel");

CKernel *CKernel::s_pThis = 0;

CKernel::CKernel (void)
:	CStdlibAppStdio ("multisynth"),
	m_GPIOManager (&mInterrupt),
 	m_I2CMaster (CMachineInfo::Get ()->GetDevice (DeviceI2CMaster), TRUE),
	m_pSPIMaster (nullptr),
	m_LCD(),
	//m_pLCDBuffered,
	m_CPUThrottle (CPUSpeedMaximum)
{
	s_pThis = this;

	// mActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel(void)
{
	s_pThis = 0;
}

bool CKernel::Initialize (void)
{
	if (!CStdlibAppStdio::Initialize ())
	{
		return FALSE;
	}

	mLogger.RegisterPanicHandler (PanicHandler);

	if (!m_GPIOManager.Initialize ())
	{
		return FALSE;
	}

	if (!m_I2CMaster.Initialize ())
	{
		return FALSE;
	}

	FATFS m_FileSystem;

	m_pConfig = new CPropertiesFatFsFile("synth.ini", &m_FileSystem);
    if (!m_pConfig->Load()) {
        LOGERR("Failed to load synth.ini");
        return FALSE;
    }

    m_pMiniDexedConfig = new CPropertiesFatFsFile("minidexed.ini", &m_FileSystem);
    if (!m_pMiniDexedConfig->Load()) {
        LOGERR("Failed to load minidexed.ini");
        return FALSE;
    }
	 
	//CPropertiesFatFsFile *m_pMiniDexedConfig = new CPropertiesFatFsFile("minidexed.ini", &fs);

	unsigned nSPIMaster = m_pConfig->GetNumber("SPIBus", SPI_INACTIVE);
	unsigned nSPIMode = m_pConfig->GetNumber ("SPIMode", SPI_DEF_MODE);
	unsigned long nSPIClock = 1000 * m_pConfig->GetNumber ("SPIClockKHz", SPI_DEF_CLOCK);
#if RASPPI<4
	// By default older RPI versions use SPI 0.
	// It is possible to build circle to support SPI 1 for
	// devices that use the 40-pin header, but that isn't
	// enabled at present...
	if (nSPIMaster == 0)
#else
	// RPI 4+ has several possible SPI Bus Configurations.
	// As mentioned above, SPI 1 is not built by default.
	// See circle/include/circle/spimaster.h
	if (nSPIMaster == 0 || nSPIMaster == 3 || nSPIMaster == 4 || nSPIMaster == 5 || nSPIMaster == 6)
#endif
	{
		unsigned nCPHA = (nSPIMode & 1) ? 1 : 0;
		unsigned nCPOL = (nSPIMode & 2) ? 1 : 0;
		m_pSPIMaster = new CSPIMaster (nSPIClock, nCPOL, nCPHA, nSPIMaster);
		if (!m_pSPIMaster->Initialize())
		{
			delete (m_pSPIMaster);
			m_pSPIMaster = nullptr;
		}
	}

	m_pUSB = new CUSBHCIDevice (&mInterrupt, &mTimer, TRUE);
	if (!m_pUSB->Initialize ())
	{
		return FALSE;
	}

	/*m_pJV880 = new CMiniJV880 (&m_Config, &mInterrupt, &m_GPIOManager, &m_I2CMaster, m_pSPIMaster, &mFileSystem, &mScreenUnbuffered);
	assert (m_pJV880);

	if (!m_pJV880->Initialize ())
	{
		return FALSE;
	}
*/
	
	InitDisplay();
	LOGNOTE("Initialisation finished");
	return TRUE;
}

CStdlibApp::TShutdownMode CKernel::Run (void)
{
	
	bool prcss = false; 
	//uint16_t cnt = 0;

	while (42 == 42)
	{
		//boolean bUpdated = m_pUSB->UpdatePlugAndPlay ();

		m_CPUThrottle.Update ();

		if (!prcss) 
		{
			LOGNOTE("Run process");
		}

		prcss = true;

		while (true) {

		}
		// m_CPUThrottle.DumpStatus ();
	}

	return ShutdownHalt;
}

void CKernel::PanicHandler (void)
{
	LOGNOTE ("panic!");

	EnableIRQs ();

	if (s_pThis->mbScreenAvailable)
	{
		s_pThis->mScreen.Update (4096);
	}
}
bool CKernel::InitDisplay (void)
    {
		unsigned i2caddr = m_pMiniDexedConfig->GetNumber("LCDI2CAddress", 0);
		unsigned ssd1306addr = m_pMiniDexedConfig->GetNumber("SSD1306LCDI2CAddress", 0x3c);
		//LOGNOTE("SSD1306 I2C Address: 0x%02X", ssd1306addr);
		bool st7789 = m_pMiniDexedConfig->GetNumber ("ST7789Enabled", 0) != 0;
		LOGNOTE("startSC55 returned: %d", m_pMiniDexedConfig->GetNumber("SSD1306LCDWidth", 128));
		LOGNOTE("startSC55 returned: %d", m_pMiniDexedConfig->GetNumber("SSD1306LCDHeight", 128));
		LOGNOTE("startSC55 returned: %d", m_pMiniDexedConfig->GetNumber("SSD1306LCDRotate", 128));
		LOGNOTE("startSC55 returned: %d", m_pMiniDexedConfig->GetNumber("SSD1306LCDMirror", 128));
		LOGNOTE("startSC55 returned: %d", ssd1306addr);
		LOGNOTE("startSC55 returned: %d", &m_I2CMaster);
		if (ssd1306addr != 0) {
			m_pSSD1306 = new CSSD1306Device (m_pMiniDexedConfig->GetNumber("SSD1306LCDWidth", 128), 
											 m_pMiniDexedConfig->GetNumber("SSD1306LCDHeight", 32),
											 &m_I2CMaster, ssd1306addr,
											 m_pMiniDexedConfig->GetNumber("SSD1306LCDRotate", 0), 
											 m_pMiniDexedConfig->GetNumber("SSD1306LCDMirror", 0));
			if (!m_pSSD1306->Initialize ())
			{
				LOGNOTE("LCD: SSD1306 initialization failed");
				return false;
			}
			LOGNOTE ("LCD: SSD1306");
			m_LCD = m_pSSD1306;
		}
		else if (st7789)
		{
			if (m_pSPIMaster == nullptr)
			{
				LOGNOTE("LCD: ST7789 Enabled but SPI Initialisation Failed");
				return false;
			}

			unsigned long nSPIClock = 1000 * m_pMiniDexedConfig->GetNumber("SPIClockKHz", SPI_DEF_CLOCK);
			unsigned nSPIMode = m_pMiniDexedConfig->GetNumber("SPIMode", SPI_DEF_MODE);
			unsigned nCPHA = (nSPIMode & 1) ? 1 : 0;
			unsigned nCPOL = (nSPIMode & 2) ? 1 : 0;
			LOGNOTE("SPI: CPOL=%u; CPHA=%u; CLK=%u",nCPOL,nCPHA,nSPIClock);
			m_pST7789Display = new CST7789Display (m_pSPIMaster,
							m_pMiniDexedConfig->GetNumber("ST7789Data", 0),
							m_pMiniDexedConfig->GetNumber("ST7789Reset", 0),
							m_pMiniDexedConfig->GetNumber("ST7789Backlight", 0 ),
							m_pMiniDexedConfig->GetNumber("ST7789Width", 240),
							m_pMiniDexedConfig->GetNumber("ST7789Height", 240),
							nCPOL, nCPHA, nSPIClock,
							m_pMiniDexedConfig->GetNumber("ST7789Select", 0));
			if (m_pST7789Display->Initialize())
			{
				m_pST7789Display->SetRotation (m_pMiniDexedConfig->GetNumber("ST7789Rotation", 0));
				bool bLargeFont = !(m_pMiniDexedConfig->GetNumber("ST7789SmallFont", 0));
				m_pST7789 = new CST7789Device (m_pSPIMaster, m_pST7789Display, 
					m_pConfig->GetNumber("LCDColumns", 16), 
					m_pConfig->GetNumber ("LCDRows", 2), 
					bLargeFont, bLargeFont);
				if (m_pST7789->Initialize())
				{
					LOGNOTE ("LCD: ST7789");
					m_LCD = m_pST7789;
				}
				else
				{
					LOGNOTE ("LCD: Failed to initalize ST7789 character device");
					delete (m_pST7789);
					delete (m_pST7789Display);
					m_pST7789 = nullptr;
					m_pST7789Display = nullptr;
					return false;
				}
			}
			else
			{
				LOGNOTE ("LCD: Failed to initialize ST7789 display");
				delete (m_pST7789Display);
				m_pST7789Display = nullptr;
				return false;
			}
		}
		else if (i2caddr == 0)
		{
			m_pHD44780 = new CHD44780Device (m_pMiniDexedConfig->GetNumber("LCDColumns", 16), 
												m_pMiniDexedConfig->GetNumber("LCDRows", 2),
												m_pMiniDexedConfig->GetNumber("LCDPinData4", 22),
												m_pMiniDexedConfig->GetNumber("LCDPinData5", 23),
												m_pMiniDexedConfig->GetNumber("LCDPinData6",24 ),
												m_pMiniDexedConfig->GetNumber("LCDPinData7", 25),
												m_pMiniDexedConfig->GetNumber("LCDPinEnable", 4),
												m_pMiniDexedConfig->GetNumber("LCDPinRegisterSelect", 27),
												m_pMiniDexedConfig->GetNumber("LCDPinReadWrite", 0));
			if (!m_pHD44780->Initialize ())
			{
				LOGNOTE("LCD: HD44780 initialization failed");
				return false;
			}
			LOGNOTE ("LCD: HD44780");
			m_LCD = m_pHD44780;
		}
		else
		{
			m_pHD44780 = new CHD44780Device (&m_I2CMaster, i2caddr,
							m_pMiniDexedConfig->GetNumber("LCDColumns", 0), m_pMiniDexedConfig->GetNumber("LCDRows", 0));
			if (!m_pHD44780->Initialize ())
			{
				LOGNOTE("LCD: HD44780 (I2C) initialization failed");
				return false;
			}
			LOGNOTE ("LCD: HD44780 I2C");
			m_LCD = m_pHD44780;
		}
		assert (m_LCD);

		m_pLCDBuffered = new CWriteBufferDevice (m_LCD);
		assert (m_pLCDBuffered);
		// clear sceen and go to top left corner
		LCDWrite ("\x1B[H\x1B[J");		// cursor home and clear screen
		LCDWrite ("\x1B[?25l\x1B""d+");		// cursor off, autopage mode
		LCDWrite ("Multisynth\nLoading...");
		m_pLCDBuffered->Update ();

		LOGNOTE ("LCD initialized");
        return true;
	}

void CKernel::UpdateDisplay() 
{
    if (!m_LCD || !m_pLCDBuffered) return;
    
    /*const char* synthNames[SYNTH_ITEM_COUNT] = {"MiniDexed", "MiniJV880", "MT-32Pi"};
    const char* currentName = synthNames[m_SelectedSynth];
    
    // Формируем строку вывода
    char displayLine[32] = {0}; // Буфер с запасом
    snprintf(displayLine, sizeof(displayLine), "%s %s %s",
             (m_SelectedSynth > 0) ? "<" : " ",
             currentName,
             (m_SelectedSynth < SYNTH_ITEM_COUNT-1) ? ">" : " ");
    
    // Очистка и вывод
	*/
    LCDWrite("\x1B[H\x1B[J"); // Clear screen
    LCDWrite("\x1B[?25l");    // Hide cursor
    LCDWrite("Select Synth\n");
    //LCDWrite(displayLine);
    
    m_pLCDBuffered->Update();
}

void CKernel::LCDWrite (const char *pString)
{
	if (m_pLCDBuffered)
	{
		m_pLCDBuffered->Write (pString, strlen (pString));
	}
}

