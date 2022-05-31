#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/serial.h>
#include <termios.h>

#include <cstring>
#include <module/MyDbg.h>

#include <module/SerialPort.h>

using namespace std;

int ALLOWEDBPS[EXTENDEDBPS_SIZE] = {
	300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};

const char *COM_NAME[COMPORT_SIZE] =
	{
		"MODEM",
		"COM1",
		"COM2",
		"COM3",
		"COM4",
		"COM5",
		"COM6"};

SpConfig gSpConfig[COMPORT_SIZE] =
	{
		SpConfig{COM_NAME[0], "/dev/ttymxc3", SpConfig::SpMode::RS232},
		SpConfig{COM_NAME[1], "/dev/ttymxc2", SpConfig::SpMode::RS485_01},
		SpConfig{COM_NAME[2], "/dev/ttymxc1", SpConfig::SpMode::RS232},
		SpConfig{COM_NAME[3], "/dev/ttymxc5", SpConfig::SpMode::RS232},
		SpConfig{COM_NAME[4], "/dev/ttymxc4", SpConfig::SpMode::RS232},
		SpConfig{COM_NAME[5], "/dev/ttySC0", SpConfig::SpMode::RS485_01},
		SpConfig{COM_NAME[6], "/dev/ttySC1", SpConfig::SpMode::RS485_01},
};

SerialPort::SerialPort(SpConfig &config)
	: spConfig(config), spFileDesc(-1)
{
	spConfig.Bytebits();
}

SerialPort::~SerialPort()
{
	Close();
}

int SerialPort::Open()
{
	// Attempt to open file
	//this->fileDesc = open(this->filePath, O_RDWR | O_NOCTTY | O_NDELAY);

	// O_RDONLY for read-only, O_WRONLY for write only, O_RDWR for both read/write access
	// 3rd, optional parameter is mode_t mode
	spFileDesc = open(spConfig.dev, O_RDWR | /*O_NOCTTY |*/ O_NONBLOCK);
	// Check status
	if (spFileDesc == -1)
	{
		return -1;
	}
	ConfigureTermios();
	return 0;
}

void SerialPort::ConfigureTermios()
{
	//================== CONFIGURE ==================//
	struct termios tty;
	bzero(&tty, sizeof(tty));
	// Get current settings (will be stored in termios structure)
	if (tcgetattr(spFileDesc, &tty) != 0)
	{
		throw std::runtime_error(FmtException("tcgetattr() failed: %s(%s)", spConfig.name, spConfig.dev));
	}
	//================= (.c_cflag) ===============//

	tty.c_cflag &= ~PARENB;		   // No parity bit is added to the output characters
	tty.c_cflag &= ~CSTOPB;		   // Only one stop-bit is used
	tty.c_cflag &= ~CSIZE;		   // CSIZE is a mask for the number of bits per character
	tty.c_cflag |= CS8;			   // Set to 8 bits per character
	tty.c_cflag &= ~CRTSCTS;	   // Disable hadrware flow control (RTS/CTS)
	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

	//===================== BAUD RATE =================//
	tty.c_cflag &= ~CBAUD;
	switch (spConfig.baudrate)
	{
	case 300:
		tty.c_cflag |= B300;
		break;
	case 600:
		tty.c_cflag |= B600;
		break;
	case 1200:
		tty.c_cflag |= B1200;
		break;
	case 2400:
		tty.c_cflag |= B2400;
		break;
	case 4800:
		tty.c_cflag |= B4800;
		break;
	case 9600:
		tty.c_cflag |= B9600;
		break;
	case 19200:
		tty.c_cflag |= B19200;
		break;
	case 38400:
		tty.c_cflag |= B38400;
		break;
	case 57600:
		tty.c_cflag |= B57600;
		break;
	case 115200:
		tty.c_cflag |= B115200;
		break;
	case 230400:
		tty.c_cflag |= B230400;
		break;
	case 460800:
		tty.c_cflag |= B460800;
		break;
	case 921600:
		tty.c_cflag |= B921600;
		break;
	default:
		throw std::invalid_argument(FmtException("%s(%s) baudrate unrecognized: %d",
												 spConfig.name, spConfig.dev, spConfig.baudrate));
	}

	//===================== (.c_oflag) =================//

	tty.c_oflag = 0; // No remapping, no delays
	//tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	//tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

	//================= CONTROL CHARACTERS (.c_cc[]) ==================//

	// c_cc[VTIME] sets the inter-character timer, in units of 0.1s.
	// Only meaningful when port is set to non-canonical mode
	// VMIN = 0, VTIME = 0: No blocking, return immediately with what is available
	// VMIN > 0, VTIME = 0: read() waits for VMIN bytes, could block indefinitely
	// VMIN = 0, VTIME > 0: Block until any amount of data is available, OR timeout occurs
	// VMIN > 0, VTIME > 0: Block until either VMIN characters have been received, or VTIME
	//                      after first character has elapsed
	// c_cc[WMIN] sets the number of characters to block (wait) for when read() is called.
	// Set to 0 if you don't want read to block. Only meaningful when port set to non-canonical mode
	// Setting both to 0 will give a non-blocking read
	tty.c_cc[VTIME] = 0;
	tty.c_cc[VMIN] = 0;

	//======================== (.c_iflag) ====================//

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

	//=========================== LOCAL MODES (c_lflag) =======================//

	// Canonical input is when read waits for EOL or EOF characters before returning. In non-canonical mode, the rate at which
	// read() returns is instead controlled by c_cc[VMIN] and c_cc[VTIME]
	tty.c_lflag &= ~ICANON; // Turn off canonical input, which is suitable for pass-through
	tty.c_lflag &= ~(ECHO);
	tty.c_lflag &= ~ECHOE;	// Turn off echo erase (echo erase only relevant if canonical input is active)
	tty.c_lflag &= ~ECHONL; //
	tty.c_lflag &= ~ISIG;	// Disables recognition of INTR (interrupt), QUIT and SUSP (suspend) characters

	// Flush port, then apply attributes
	tcflush(spFileDesc, TCIOFLUSH);
	if (tcsetattr(spFileDesc, TCSANOW, &tty) != 0)
	{
		throw std::runtime_error(FmtException("tcsetattr() failed: %s(%s): (%d): %s\n",
											  spConfig.name, spConfig.dev, errno, strerror(errno)));
	}

	struct serial_rs485 rs485conf;
	if (ioctl(spFileDesc, TIOCGRS485, &rs485conf) < 0)
	{
		throw std::runtime_error(FmtException("Error reading ioctl %s(%s): (%d): %s\n",
											  spConfig.name, spConfig.dev, errno, strerror(errno)));
	}
	rs485conf.flags = 0;
	if (spConfig.mode == SpConfig::SpMode::RS232)
	{
		rs485conf.flags &= ~SER_RS485_ENABLED;
	}
	else
	{
		rs485conf.flags |= SER_RS485_ENABLED |
						   (spConfig.mode == SpConfig::SpMode::RS485_10 ? SER_RS485_RTS_ON_SEND : SER_RS485_RTS_AFTER_SEND);
	}
	if (ioctl(spFileDesc, TIOCSRS485, &rs485conf) < 0)
	{
		throw std::runtime_error(FmtException("Error writing ioctl %s(%s): (%d): %s\n",
											  spConfig.name, spConfig.dev, errno, strerror(errno)));
	}
	if (ioctl(spFileDesc, TIOCGRS485, &rs485conf) < 0)
	{
		throw std::runtime_error(FmtException("Error reading ioctl %s(%s): (%d): %s\n",
											  spConfig.name, spConfig.dev, errno, strerror(errno)));
	}
	/*
	if(spConfig.mode!=SpConfig::SpMode::RS232)
	{
		Ldebug("Confirm RS485 mode is %s", (rs485conf.flags & SER_RS485_ENABLED) ? "set" : "NOT set");
	}
	*/
}

int SerialPort::Close()
{
	if (spFileDesc > 0)
	{
		auto retVal = close(spFileDesc);
		if (retVal != 0)
		{
			throw std::runtime_error(FmtException("Close() failed: %s(%s)", spConfig.name, spConfig.dev));
		}
		spFileDesc = -1;
	}
	return 0;
}
