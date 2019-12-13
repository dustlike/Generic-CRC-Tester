#include<cstdio>
#include<cstdint>
#include<cstring>
#include<vector>

using std::vector;


/////////////////////


#define CRC_WIDTH		16
#define POLYNOMIAL		0x8005
#define REFLECT_IO		1
#define INIT_VALUE		0xFFFF
#define FINAL_XOR		0


#if CRC_WIDTH < 8 || CRC_WIDTH > 32
#error "CRC_WITDH must be between 8 and 32"
#endif

#if (CRC_WIDTH == 32)
#  define BIT_MASK		0xFFFFFFFF
#else
#  define BIT_MASK		((1 << CRC_WIDTH) - 1)
#endif


/////////////////////


#if (CRC_WIDTH == 8)
#  define CRC_FORMAT		"%02X"

#elif (CRC_WIDTH <= 16)
#  define CRC_FORMAT		"%04X"

#else
#  define CRC_FORMAT		"%08X"

#endif


uint32_t crc_table[256];
uint32_t polynomial;


void setup_crc()
{
	polynomial = POLYNOMIAL;
	
#if (REFLECT_IO == 1)
	//reverse the polynomial
	int width = CRC_WIDTH - 1;
	uint32_t rev_poly = POLYNOMIAL;
	
	polynomial >>= 1;
	
	while (polynomial)
	{
		rev_poly <<= 1;
		rev_poly |= polynomial & 1;
		polynomial >>= 1;
		width--;
	}
	
	polynomial = (rev_poly << width) & BIT_MASK;
#endif
	
	//create look-up table
	for (uint32_t i = 0; i < 256; i++)
	{
		uint32_t crc;
		
#if REFLECT_IO == 1
		// LSB first
		crc = i;
		
		for (int j=0; j<8; j++)
		{
			uint32_t tmp = crc >> 1;
			if (crc & 1) tmp ^= polynomial;
			crc = tmp;
		}
#else
		// MSB first
		crc = i << (CRC_WIDTH - 8);
		
		for (int j=0; j<8; j++)
		{
			uint32_t tmp = crc << 1;
			if (crc & (1 << (CRC_WIDTH - 1))) tmp ^= polynomial;
			crc = tmp;
		}
#endif
		
		crc_table[i] = crc & BIT_MASK;
	}
}


uint32_t calc_crc_table(const vector<uint8_t>& data)
{
	uint32_t crc = INIT_VALUE;
	
	for (size_t i = 0; i < data.size(); i++)
	{
#if REFLECT_IO == 1
		// LSB first
		uint8_t next = data[i] ^ crc;
		crc = (crc >> 8) ^ crc_table[next];
#else
		// MSB first
		uint8_t next = data[i] ^ (crc >> (CRC_WIDTH - 8));
		crc = (crc << 8) ^ crc_table[next];
#endif
	}
	
	return (crc ^ FINAL_XOR) & BIT_MASK;
}


uint32_t calc_crc_normal(const vector<uint8_t>& data)
{
	uint32_t crc = INIT_VALUE;
	
	for (size_t i = 0; i < data.size(); i++)
	{
#if REFLECT_IO == 1
		// LSB first
		crc ^= data[i];
		
		for (int j=0; j<8; j++)
		{
			uint32_t tmp = crc >> 1;
			if (crc & 1) tmp ^= polynomial;
			crc = tmp;
		}
#else
		// MSB first
		crc ^= data[i] << (CRC_WIDTH - 8);
		
		for (int j=0; j<8; j++)
		{
			uint32_t tmp = crc << 1;
			if (crc & (1 << (CRC_WIDTH - 1))) tmp ^= polynomial;
			crc = tmp;
		}
#endif
	}
	
	return (crc ^ FINAL_XOR) & BIT_MASK;
}


/////////////////////


int hex_to_int(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	else if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	else if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	return -1;
}


void dump_data_array(const vector<uint8_t>& data)
{
	vector<uint8_t>::const_iterator p = data.begin();
	
	printf("total %u bytes", data.size());
	
	size_t counter = 0;
	for (; p != data.end(); p++)
	{
		printf("%c%02X", counter % 16 == 0? '\n' : ' ', *p);
		counter++;
	}
	
	printf("\n");
}


void show_new_byte_count(unsigned *count)
{
	if (*count > 0)
	{
		printf("add %u bytes\n", *count);
		*count = 0;	
	}
}


void print_crc_info()
{
	printf("Width: %d-bits\n", CRC_WIDTH);
	printf("Polynomial:  0x" CRC_FORMAT "\n", POLYNOMIAL);
#if (REFLECT_IO == 1)
	printf("Relfect: Yes\n");
#else
	printf("Relfect: No\n");
#endif
	printf("Initial: %X\n", INIT_VALUE);
	printf("FinalXOR: %X\n", FINAL_XOR);
}


void print_crc_table()
{
	size_t j = 0;
	
	for (size_t i = 0; i < 256; i++)
	{
		printf("0x" CRC_FORMAT ", ", crc_table[i]);
		
		if (++j >= 16)
		{
			printf("\n");
			j = 0;
		}
	}
}


int main()
{
	setup_crc();
	
	char line[1024];
	vector<uint8_t> data;
	bool second_digit = false;
	uint8_t tmp = 0;
	unsigned new_bytes = 0;
	
	while (1)
	{
		printf("> ");
		fflush(stdout);
		
		if (!fgets(line, sizeof(line), stdin)) break;
		
		size_t n = strlen(line);
		
		for (size_t i = 0; i < n; i++)
		{
			int d = hex_to_int(line[i]);
			
			if (d >= 0 && d < 16)	//add new data
			{
				if (second_digit)
				{
					data.push_back(tmp | d);
					new_bytes++;
					
					second_digit = false;
					tmp = 0;
				}
				else
				{
					tmp = d << 4;
					second_digit = true;
				}
			}
			else if (line[i] == '?')
			{
				show_new_byte_count(&new_bytes);
				dump_data_array(data);
			}
			else if (line[i] == '-')
			{
				show_new_byte_count(&new_bytes);
				printf("CLEAR ALL\n");
				data.clear();
			}
			else if (line[i] == '#')
			{
				show_new_byte_count(&new_bytes);
				uint32_t crc = calc_crc_normal(data);
				
				printf("Normal Method\n");
				print_crc_info();
				printf("result = 0x" CRC_FORMAT "\n", crc);
			}
			else if (line[i] == '@')
			{
				show_new_byte_count(&new_bytes);
				uint32_t crc = calc_crc_table(data);
				
				printf("Look-up Table Method\n");
				print_crc_info();
				printf("result = 0x" CRC_FORMAT "\n", crc);
			}
			else if (line[i] == '&')
			{
				print_crc_table();
			}
		}
		
		show_new_byte_count(&new_bytes);
		
		printf("\n");
	}
	
	return 0;
}

