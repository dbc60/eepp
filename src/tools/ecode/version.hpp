#ifndef ECODE_VERSION_HPP
#define ECODE_VERSION_HPP

#include <eepp/config.hpp>
#include <string>

using namespace EE;

#define ECODE_MAJOR_VERSION 0
#define ECODE_MINOR_VERSION 4
#define ECODE_PATCH_LEVEL 0
#define ECODE_CODENAME "Vajra"

/** The compiled version of the library */
#define ECODE_VERSION( x )                  \
	{                                       \
		( x )->major = ECODE_MAJOR_VERSION; \
		( x )->minor = ECODE_MINOR_VERSION; \
		( x )->patch = ECODE_PATCH_LEVEL;   \
	}

#define ECODE_VERSIONNUM( X, Y, Z ) ( (X)*1000 + (Y)*100 + ( Z ) )

#define ECODE_COMPILEDVERSION \
	ECODE_VERSIONNUM( ECODE_MAJOR_VERSION, ECODE_MINOR_VERSION, ECODE_PATCH_LEVEL )

#define ECODE_VERSION_ATLEAST( X, Y, Z ) ( ECODE_COMPILEDVERSION >= ECODE_VERSIONNUM( X, Y, Z ) )

namespace ecode {

class Version {
  public:
	Uint8 major; /**< major version */
	Uint8 minor; /**< minor version */
	Uint8 patch; /**< update version */

	/** @return The linked version of the library */
	static Version getVersion();

	/** @return The linked version number of the library */
	static Uint32 getVersionNum();

	/** @return The library version name: "eepp version major.minor.patch" */
	static std::string getVersionName();

	/** @return The version codename */
	static std::string getCodename();

	/** @return The build time of the library */

	static inline std::string getBuildTime() {
		return std::string( __DATE__ ) + " " + std::string( __TIME__ );
	}
};

} // namespace ecode

#endif
