#include <vector>


std::vector<uint8_t> to_utf8(const std::vector<uint32_t>& x) {
	std::vector<uint8_t> res;
	for (auto& el : x) {
		if (el < 128) {
			res.push_back(static_cast<uint8_t>(el));
		} else if (el < 2048) {
			uint8_t b1 = static_cast<uint8_t>(el >> 6 & (1 << 5) - 1) + 0xC0;
			uint8_t b2 = static_cast<uint8_t>(el & (1 << 6) - 1) + 0x80;
			res.push_back(b1);
			res.push_back(b2);
		} else if (el < 65536) {
			uint8_t b1 = static_cast<uint8_t>(el >> 12 & (1 << 4) - 1) + 0xE0;
			uint8_t b2 = static_cast<uint8_t>(el >> 6 & (1 << 6) - 1) + 0x80;
			uint8_t b3 = static_cast<uint8_t>(el & (1 << 6) - 1) + 0x80;
			res.push_back(b1);
			res.push_back(b2);
			res.push_back(b3);
		} else {
			uint8_t b1 = static_cast<uint8_t>(el >> 18 & (1 << 3) - 1) + 0xF0;
			uint8_t b2 = static_cast<uint8_t>(el >> 12 & (1 << 6) - 1) + 0x80;
			uint8_t b3 = static_cast<uint8_t>(el >> 6 & (1 << 6) - 1) + 0x80;
			uint8_t b4 = static_cast<uint8_t>(el & (1 << 6) - 1) + 0x80;
			res.push_back(b1);
			res.push_back(b2);
			res.push_back(b3);
			res.push_back(b4);
		}
	}
	return res;
}


std::vector<uint32_t> from_utf8 (const std::vector<uint8_t>& x) {
	std::vector<uint32_t> res;
	uint8_t b1, b2, b3, b4;
	uint32_t b_res;
	for (int i = 0; i < x.size(); i++) {
		if (x[i] < 128) {
			res.push_back(x[i]);
		} else {
			bool stop = false;
			int offset = 0;
			while(!stop) {
				offset++;
				stop = !(x[i] & (1 << (7 - offset)));
			}
			switch(offset) {
				case 2:
					b1 = x[i] & 0x1F;
					b2 = x[i+1] & 0x3F;
					b_res = b1 * 0x40 + b2;
					res.push_back(b_res);
					break;
				case 3:
					b1 = x[i] & 0x0F;
					b2 = x[i+1] & 0x3F;
					b3 = x[i+2] & 0x3F;
					b_res = b1 * 0x1000 + b2 * 0x40 + b3;
					res.push_back(b_res);
					break;
				case 4:
					b1 = x[i] & 0x07;
					b2 = x[i+1] & 0x3F;
					b3 = x[i+2] & 0x3F;
					b4 = x[i+3] & 0x3F;
					b_res = b1 * 0x40000 + b2 * 0x1000 + b3 * 0x40 + b4;
					res.push_back(b_res);
					break;
			}
			i += offset - 1;
		}
	}
	return res;
}


int main(int argc, char const *argv[])
{
	std::vector<uint32_t> unicode_instance = {1, 256, 1000, 25555, 67000};
	auto unicode_dec = from_utf8(to_utf8(unicode_instance));
	for (const auto& el: unicode_dec) {
		printf("%d\n", static_cast<uint32_t>(el));
	}
	return 0;
}

