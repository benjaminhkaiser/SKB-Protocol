#include <util.h>
#include <string>
#include <sstream>
#include <limits>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>

std::string makeHash(const std::string& input)
{
	CryptoPP::SHA512 hash;
	byte digest[ CryptoPP::SHA512::DIGESTSIZE ];

	hash.CalculateDigest( digest, (byte*) input.c_str(), input.length() );

	CryptoPP::HexEncoder encoder;
	std::string output;
	encoder.Attach( new CryptoPP::StringSink( output ) );
	encoder.Put( digest, sizeof(digest) );
	encoder.MessageEnd();

	return output;
}

std::string to_string(int number)
{
   std::stringstream ss;
   ss << number;
   return ss.str();
}

std::string randomString(const int len)
{
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string s = "";
    for (int i = 0; i < len; ++i) {
        s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return s;
}

bool doubleOverflow(const double& x, const double& y)
{
	double max = std::numeric_limits<double>::max();
	double min = std::numeric_limits<double>::min();

	if(y > 0 && x > 0)
	{
		if(y <= max - x)
		{
			return false;
		} else {
			return true;
		}
	} else if(x < 0 && y < 0) {
		if(y >= min - x)
		{
			return false;
		} else {
			return true;
		}
	} else {
		return false;
	}
}