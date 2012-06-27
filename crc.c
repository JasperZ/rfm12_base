#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

uint8_t crc8(uint8_t *daten, uint8_t size, uint8_t polynom)
{
	uint16_t offset;
	uint8_t *ptr;
	uint8_t polynomlaenge;
	uint8_t tmp, last;
	uint8_t* rahmenUndAnhang;
	
	polynomlaenge = 1;
	last = 0;
	
	rahmenUndAnhang = (uint8_t*)malloc(size+polynomlaenge);

	memset(rahmenUndAnhang, 0x00, size+polynomlaenge);
	memmove(rahmenUndAnhang, daten, size);

	offset = 0;
	ptr = rahmenUndAnhang;

	while(offset < size*8)
	{
		tmp = offset/8;
		ptr = rahmenUndAnhang+tmp;
		tmp = offset%8;
		
		if(tmp > 0)
		{
			tmp = (*ptr<<tmp) | (*(ptr+1)>>(8-tmp));
		}
		else
		{
			tmp = *ptr;
		}
		
		last = tmp ^ polynom;
		tmp = offset%8;
		
		if(tmp > 0) 
		{
			*ptr = last>>tmp;
			*(ptr+1) = last<<(8-tmp) | (*(ptr+1) & (255>>tmp));
		}
		else
		{
			*ptr = last;
		}

		if((128&last) == 0)
		{
			offset++;
			
		}
	}

	free(rahmenUndAnhang);
	
	return last;
}

uint8_t checkCrc8(uint8_t *daten, uint8_t size, uint8_t rest, uint8_t polynom)
{
	uint8_t i;
	uint16_t offset;
	uint8_t *ptr;
	uint8_t polynomlaenge;
	uint8_t tmp, last;
	uint8_t* rahmenUndAnhang;
	
	polynomlaenge = 1;
	last = 0;
	
	rahmenUndAnhang = (uint8_t*)malloc(size+polynomlaenge);

	memset(rahmenUndAnhang, 0x00, size+polynomlaenge);
	memmove(rahmenUndAnhang, daten, size);
	
	offset = 0;
	
	for(i=128; i>=1; i/=2)
	{
		if((i&rest) == 0)
		{
			offset++;
		}
		else
		{
			break;
		}
	}
	
	tmp = 8-offset;
	
	*(rahmenUndAnhang+size) = rest<<(offset-(7-tmp));
	
	offset = 0;
	ptr = rahmenUndAnhang;

	while(offset < size*8)
	{
		tmp = offset/8;
		ptr = rahmenUndAnhang+tmp;
		tmp = offset%8;
		
		if(tmp > 0)
		{
			tmp = (*ptr<<tmp) | (*(ptr+1)>>(8-tmp));
		}
		else
		{
			tmp = *ptr;
		}
		
		last = tmp ^ polynom;
		tmp = offset%8;
		
		if(tmp > 0) 
		{
			*ptr = last>>tmp;
			*(ptr+1) = last<<(8-tmp) | (*(ptr+1) & (255>>tmp));
		}
		else
		{
			*ptr = last;
		}

		if((128&last) == 0)
		{
			offset++;
		}
	}

	free(rahmenUndAnhang);
	
	return last;
}
