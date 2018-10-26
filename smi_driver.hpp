#ifndef _SMI_DRIVER_H
#define _SMI_DRIVER_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

/*MDC: gpio352, MDIO: gpio353*/
#define READ_OPT 2
#define WRITE_OPT 1

class GPIO{
	private: 
			int exportfd;
			int directionfd;
			int valuefd;
			char filename[128];
	public:
			void export_file(int number)
				{
					exportfd = open("sys/class/gpio/export", O_WRONLY);
					if(exportfd < 0)
						{
							printf("can't open gpio to export it\n");
							exit(1);
						}
					write(exportfd, &number, 4);
					close(exportfd);
				}
			void set_direction(int number, const char *direction)
				{
					sprintf(filename, "/sys/class/gpio/gpio%d/direction", number);
					directionfd = open(filename, O_RDWR);
					if (directionfd < 0)
    					{
        					printf("Can't open GPIO direction it\n");
        					exit(1);
   						 }
					write(directionfd, &direction, (strlen(direction) + 1));
					close(directionfd);
				}
			void set_value(int number, int value)
				{
					sprintf(filename, "/sys/class/gpio/gpio%d/value", number);
					valuefd = open(filename, O_RDWR);
					if (valuefd < 0)
    					{
        					printf("Can't open GPIO value\n");
        					exit(1);
   						 }
					write(valuefd, &value, 2);
					close(valuefd);
				}
			int get_value(int number)
				{
					sprintf(filename, "/sys/class/gpio/gpio%d/value", number);
					valuefd = open(filename, O_RDWR);
					if (valuefd < 0)
    					{
        					printf("Can't open GPIO value\n");
        					exit(1);
   						 }
					int value;
					read(valuefd, &value, 2);
					close(valuefd);
					return value;
				}
			void nanosleep_configure()//delay 100ns
			{
				struct timespec ts;
				ts.tv_sec = 0;
				ts.tv_nsec = 100;
				nanosleep(&ts, NULL);
			}
			
};
GPIO gpio;
void GPIO_Init()
{
	gpio.export_file(352);
	gpio.export_file(353);
	gpio.set_direction(352, "out");
	//gpio.set_direction(353, "in");
}
void MDC_CLK()
{
	GPIO_Init();
	gpio.set_value(352, 0);
	gpio.nanosleep_configure();//delay 100 ns, clk frequency = 5MHz
	gpio.set_value(352, 1);
	gpio.nanosleep_configure();//delay 100 ns
}
void MDIO_Write_Bit(int value)
{
	GPIO_Init();
	gpio.set_direction(353, "out");
	gpio.set_value(353, value);
	gpio.nanosleep_configure();

	gpio.set_value(352, 1);
	gpio.nanosleep_configure();
	gpio.set_value(352, 0);
	//MDC_CLK();
}
int MDIO_Read_Bit()
{
	GPIO_Init();
	gpio.set_direction(353, "in");
	gpio.nanosleep_configure();
	//MDC_CLK();
	gpio.set_value(352, 1);
	gpio.nanosleep_configure();
	gpio.set_value(352, 0);

	int value = gpio.get_value(353);

	return value;
}
void MDIO_Write_Bits_number(int data, int bits)
{
	int i;
	for(i = bits - 1 ; i >= 0; i --)
	{
		MDIO_Write_Bit((data >> i) & 1);
	}
}
int MDIO_Read_Bits_number(int bits)
{
	int i;
	int ret = 0;
	for(i = bits - 1; i >= 0; i--)
	{
		ret <<= 1;
		ret |= MDIO_Read_Bit();
	}
	return ret;
}
void SMI_Configure(int opt, int phy_addr, int reg_addr)
{
	GPIO_Init();
	gpio.set_direction(353, "out");

	MDIO_Write_Bit(0);//st:01
	MDIO_Write_Bit(1);

	MDIO_Write_Bit((opt >> 1) & 1);//read_opt=2(10),write_opt=1(01)
	MDIO_Write_Bit((opt >> 0) & 1);

	MDIO_Write_Bits_number(phy_addr, 5);
	MDIO_Write_Bits_number(reg_addr, 5);
}
void MDIO_Write(int phy_addr, int reg_addr, int data)
{
	GPIO_Init();
	SMI_Configure(WRITE_OPT, phy_addr, reg_addr);

	MDIO_Write_Bit(1);//write turnaround 10
	MDIO_Write_Bit(0);

	MDIO_Write_Bits_number(data, 16);

	gpio.set_direction(353, "in");
	MDIO_Read_Bit();
}
int MDIO_Read(int phy_addr, int reg_addr)
{
	GPIO_Init();
	SMI_Configure(READ_OPT, phy_addr, reg_addr);
	gpio.set_direction(353, "in");

	/* check the turnaround bit: the PHY should be driving it to zero, if this
	 * PHY is listed in phy_ignore_ta_mask as having broken TA, skip that
	 */
	if(MDIO_Read_Bit() != 0)
	{
		for(int i = 0; i < 32; i++)
		{
			MDIO_Read_Bit();
			return 0xFFFF;
		}
	}
	int ret = MDIO_Read_Bits_number(16);
	MDIO_Read_Bit();

	return ret;
}
/*int main()
{
	return 0;
}*/
#endif