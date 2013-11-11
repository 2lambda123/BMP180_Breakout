/*
  BMP180 pressure sensor library
	
  Mike Grusin
  http://www.sparkfun.com

  Uses floating-point equations from the Weather Station Data Logger project
  http://wmrx00.sourceforge.net/
  http://wmrx00.sourceforge.net/Arduino/BMP085-Calcs.pdf

  Forked from BMP085 library by M.Grusin

	Our example code uses the "beerware" license. You can do anything
	you like with this code. No really, anything. If you find it useful,
	buy me a beer someday.

	version 1.0 2013/09/20 initial version
	
  Example Code:

  #include <SFE_BMP180.h>
  #include <Wire.h>

  SFE_BMP180 pressure(BMP180_ADDR);

  char status;
  double T,P,p0,a;

  Serial.begin(9600);
  Serial.println("REBOOT");

  // initialize the sensor (important to get calibration values stored on the device)
  if (pressure.begin())
  {
    Serial.println("BMP180 init success");
    while(1)
    {
      // tell the sensor to start a temperature measurement
      // if request is successful, the number of ms to wait is returned
      // if request is unsuccessful, 0 is returned
      status = pressure.startTemperature();
      if (status != 0)
      {
				// wait for the measurement to complete
				delay(status);
				// retrieve the measurement
				// note that the measurement is stored in the variable T
				// use '&T' to provide the address of T to the function
				// function returns 1 if successful, 0 if failure
				status = pressure.getTemperature(&T);
				if (status != 0)
				{
          // print out the measurement
          Serial.print("temp: ");
          Serial.print(T,2);
          Serial.println(" deg C");

          // tell the sensor to start a pressure measurement
          // the parameter is the oversampling setting, from 0 to 3 (highest res, longest wait)
          // if request is successful, the number of ms to wait is returned
          // if request is unsuccessful, 0 is returned
          status = pressure.startPressure(3);
          if (status != 0)
          {
            // wait for the measurement to complete
            delay(status);
            // retrieve the measurement
            // note that the measurement is stored in the variable P
            // use '&P' to provide the address of P
            // note also that the function requires the previous temperature measurement (T)
            // (if temperature is stable, you can do one temperature measurement for a number of pressure measurements)
            // function returns 1 if successful, 0 if failure
            status = pressure.getPressure(&P,&T);
            if (status != 0)
            {
              // print out the measurement
              Serial.print("pressure: ");
              Serial.print(P,2);
              Serial.println(" mb");

              // the pressure sensor returns abolute pressure, which varies with altitude
              // to remove the effects of altitude, use the sealevel function and your current altitude
              // this number is commonly use in published weather data
              p0 = pressure.sealevel(P,1655.0); // we're at 1655 meters (Boulder, CO)
              Serial.print("sea-level pressure: ");
              Serial.print(p0,2);
              Serial.println(" mb");

              // on the other hand, if you want to determine your altitude from the pressure reading,
              // use the altitude function along with a baseline pressure (sea-level or other)
              a = pressure.altitude(P,p0);
              Serial.print("altitude above sea-level: ");
              Serial.print(a,0);
              Serial.println(" meters");
            }
            else Serial.println("error retrieving pressure measurement\n");
          }
          else Serial.println("error starting pressure measurement\n");
        }
        else Serial.println("error retrieving temperature measurement\n");
      }
      else Serial.println("error starting temperature measurement\n");

      delay(10000);
    }
  }
  else Serial.println("BMP180 init fail\n\n");


*/

#include <SFE_BMP180.h>
#include <Wire.h>
#include <stdio.h>
#include <math.h>

SFE_BMP180::SFE_BMP180(char i2c_address)
{
	_i2c_address = i2c_address;
	//	Wire.begin();
}

char SFE_BMP180::begin()
	// call pressure.begin() to initialize BMP180 before use
	// returns 1 if success, 0 if failure (bad component or I2C bus shorted?)
{
	double c3,c4,b1;
	
	Wire.begin();

	// get calibration data
	
	if (readInt(0xAA,&AC1) &&
		readInt(0xAC,&AC2) &&
		readInt(0xAE,&AC3) &&
		readUInt(0xB0,&AC4) &&
		readUInt(0xB2,&AC5) &&
		readUInt(0xB4,&AC6) &&
		readInt(0xB6,&VB1) &&
		readInt(0xB8,&VB2) &&
		readInt(0xBA,&MB) &&
		readInt(0xBC,&MC) &&
		readInt(0xBE,&MD))
	{

		// all reads completed successfully!
		// compute floating-point polynominals
		
		// example from Bosch datasheet
		//AC1 = 408; AC2 = -72; AC3 = -14383; AC4 = 32741; AC5 = 32757; AC6 = 23153;
		//B1 = 6190; B2 = 4; MB = -32768; MC = -8711; MD = 2868;

		// example from http://wmrx00.sourceforge.net/Arduino/BMP180-Calcs.pdf
		//AC1 = 7911; AC2 = -934; AC3 = -14306; AC4 = 31567; AC5 = 25671; AC6 = 18974;
		//VB1 = 5498; VB2 = 46; MB = -32768; MC = -11075; MD = 2432;

		/*		
		Serial.print("AC1: "); Serial.println(AC1);
		Serial.print("AC2: "); Serial.println(AC2);
		Serial.print("AC3: "); Serial.println(AC3);
		Serial.print("AC4: "); Serial.println(AC4);
		Serial.print("AC5: "); Serial.println(AC5);
		Serial.print("AC6: "); Serial.println(AC6);
		Serial.print("VB1: "); Serial.println(VB1);
		Serial.print("VB2: "); Serial.println(VB2);
		Serial.print("MB: "); Serial.println(MB);
		Serial.print("MC: "); Serial.println(MC);
		Serial.print("MD: "); Serial.println(MD);
		*/

		c3 = 160.0 * pow(2,-15) * AC3;
		c4 = pow(10,-3) * pow(2,-15) * AC4;
		b1 = pow(160,2) * pow(2,-30) * VB1;
		c5 = (pow(2,-15) / 160) * AC5;
		c6 = AC6;
		mc = (pow(2,11) / pow(160,2)) * MC;
		md = MD / 160.0;
		x0 = AC1;
		x1 = 160.0 * pow(2,-13) * AC2;
		x2 = pow(160,2) * pow(2,-25) * VB2;
		y0 = c4 * pow(2,15);
		y1 = c4 * c3;
		y2 = c4 * b1;
		p0 = (3791.0 - 8.0) / 1600.0;
		p1 = 1.0 - 7357.0 * pow(2,-20);
		p2 = 3038.0 * 100.0 * pow(2,-36);

		/*
		Serial.println();
		Serial.print("c3: "); Serial.println(c3);
		Serial.print("c4: "); Serial.println(c4);
		Serial.print("c5: "); Serial.println(c5);
		Serial.print("c6: "); Serial.println(c6);
		Serial.print("b1: "); Serial.println(b1);
		Serial.print("mc: "); Serial.println(mc);
		Serial.print("md: "); Serial.println(md);
		Serial.print("x0: "); Serial.println(x0);
		Serial.print("x1: "); Serial.println(x1);
		Serial.print("x2: "); Serial.println(x2);
		Serial.print("y0: "); Serial.println(y0);
		Serial.print("y1: "); Serial.println(y1);
		Serial.print("y2: "); Serial.println(y2);
		Serial.print("p0: "); Serial.println(p0);
		Serial.print("p1: "); Serial.println(p1);
		Serial.print("p2: "); Serial.println(p2);
		*/
		
		return(1);
	}
	else
	{
		// error reading calibration data; bad component or connection?
		return(0);
	}
}


char SFE_BMP180::readInt(char address, int *value)
	//value is address of a short (16-bit) int
{
	unsigned char data[2];

	data[0] = address;
	if (readBytes(data,2))
	{
		*value = (((int)data[0]<<8)|(int)data[1]);
		//if (*value & 0x8000) *value |= 0xFFFF0000; // sign extend if negative
		return(1);
	}
	*value = 0;
	return(0);
}


char SFE_BMP180::readUInt(char address, unsigned int *value)
	//value is address of a short (16-bit) int
{
	unsigned char data[2];

	data[0] = address;
	if (readBytes(data,2))
	{
		*value = (((unsigned int)data[0]<<8)|(unsigned int)data[1]);
		return(1);
	}
	*value = 0;
	return(0);
}


char SFE_BMP180::readBytes(unsigned char *values, char length)
	//values is an array of char, first entry should be the register to read from
	//subsequent entries will be filled with return values
{
	char x;

	Wire.beginTransmission(_i2c_address);
	Wire.write(values[0]);
	if (Wire.endTransmission() == 0);
	{
		Wire.requestFrom(_i2c_address,length);
		while(Wire.available() != length) ; // wait until bytes are ready
		for(x=0;x<length;x++)
		{
			values[x] = Wire.read();
		}
		return(1);
	}
	return(0);
}


char SFE_BMP180::writeBytes(unsigned char *values, char length)
	//value is an array of char, first entry should be the register to write to
	//subsequent entries will be values to write beginning at that register
{
	char x;
	
	Wire.beginTransmission(_i2c_address);
	Wire.write(values,length);
	if (Wire.endTransmission() == 0)
		return(1);
	else
		return(0);
}


char SFE_BMP180::startTemperature(void)
	// command BMP180 to start a temperature measurement
	// returns n (number of ms to wait) for success, 0 for fail
{
	unsigned char data[2], result;
	
	data[0] = BMP180_REG_CONTROL;
	data[1] = BMP180_COMMAND_TEMPERATURE;
	result = writeBytes(data, 2);
	if (result) // good write?
		return(5); // return the delay in ms (rounded up) to wait before retrieving data
	else
		return(0); // or return 0 if there was a problem communicating with the BMP
}


char SFE_BMP180::getTemperature(double *T)
	// return temperature measurement from previous startTemperature command
	// places returned value in T variable (deg C)
	// returns 1 for success, 0 for fail
{
	unsigned char data[2];
	char result;
	double tu, a;
	//char tempstring[20];
	
	data[0] = BMP180_REG_RESULT;

	result = readBytes(data, 2);
	if (result) // good read, calculate temperature
	{
		tu = (data[0] * 256.0) + data[1];

		//example from Bosch datasheet
		//tu = 27898;

		//example from http://wmrx00.sourceforge.net/Arduino/BMP085-Calcs.pdf
		//tu = 0x69EC;
		
		a = c5 * (tu - c6);
		*T = a + (mc / (a + md));

		/*		
		Serial.println();
		Serial.print("tu: "); Serial.println(tu);
		Serial.print("a: "); Serial.println(a);
		Serial.print("T: "); Serial.println(*T);
		*/
	}
	return(result);
}


char SFE_BMP180::startPressure(char oversampling)
	// command BMP180 to start a pressure measurement
	// oversampling: 0 - 3 for oversampling value
	// returns n (number of ms to wait) for success, 0 for fail
{
	unsigned char data[2], result, delay;
	
	data[0] = BMP180_REG_CONTROL;

	switch (oversampling)
	{
		case 0:
			data[1] = BMP180_COMMAND_PRESSURE0;
			delay = 5;
		break;
		case 1:
			data[1] = BMP180_COMMAND_PRESSURE1;
			delay = 8;
		break;
		case 2:
			data[1] = BMP180_COMMAND_PRESSURE2;
			delay = 14;
		break;
		case 3:
			data[1] = BMP180_COMMAND_PRESSURE3;
			delay = 26;
		break;
		default:
			data[1] = BMP180_COMMAND_PRESSURE0;
			delay = 5;
		break;
	}
	result = writeBytes(data, 2);
	if (result) // good write?
		return(delay); // return the delay in ms (rounded up) to wait before retrieving data
	else
		return(0); // or return 0 if there was a problem communicating with the BMP
}


char SFE_BMP180::getPressure(double *P, double *T)
	// return absolute pressure measurement in mbars from previous startPressure command
	// note: requires previous temperature measurement in variable T
	// places returned value in P variable (mbar)
	// returns 1 for success, 0 for fail

	// note that parameters are pointers to variables, call with "&var" to send addresses
	// requires begin() to have been called once to retrieve calibration parameters
	// requires recent temperature reading (startTemperature() / getTemperature()) to calculate pressure
	// requires startPressure() to have been called prior to calling getPressure()
	// return value will be 1 for success, 0 for I2C error
	// note that calculated pressure value is absolute mbars, to compensate for altitude call sealevel()
{
	unsigned char data[3];
	char result;
	double pu,s,x,y,z;
	//char tempstring[20];
	
	data[0] = BMP180_REG_RESULT;

	result = readBytes(data, 3);
	if (result) // good read, calculate pressure
	{
		pu = (data[0] * 256.0) + data[1] + (data[2]/256.0);

		//example from Bosch datasheet
		//pu = 23843;

		//example from http://wmrx00.sourceforge.net/Arduino/BMP085-Calcs.pdf, pu = 0x982FC0;	
		//pu = (0x98 * 256.0) + 0x2F + (0xC0/256.0);
		
		s = *T - 25.0;
		x = (x2 * pow(s,2)) + (x1 * s) + x0;
		y = (y2 * pow(s,2)) + (y1 * s) + y0;
		z = (pu - x) / y;
		*P = (p2 * pow(z,2)) + (p1 * z) + p0;

		/*
		Serial.println();
		Serial.print("pu: "); Serial.println(pu);
		Serial.print("T: "); Serial.println(*T);
		Serial.print("s: "); Serial.println(s);
		Serial.print("x: "); Serial.println(x);
		Serial.print("y: "); Serial.println(y);
		Serial.print("z: "); Serial.println(z);
		Serial.print("P: "); Serial.println(*P);
		*/
	}
	return(result);
}


double SFE_BMP180::sealevel(double P, double A)
	// convert absolute pressure to sea-level pressure (as used in weather data)
	// P: absolute pressure (mbar)
	// A: current altitude (meters)
	// returns sealevel pressure in mbar
{
	return(P / pow(1-(A/44330.0),5.255));
}


double SFE_BMP180::altitude(double P, double P0)
	// convert absolute pressure to altitude (given baseline pressure; sea-level, runway, etc.)
	// P: absolute pressure (mbar)
	// P0: fixed baseline pressure (mbar)
	// returns signed altitude in meters
{
	return(44330.0*(1-pow(P/P0,1/5.255)));
}
