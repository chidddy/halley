#pragma once

#include "halley/text/halleystring.h"
#include "halley/utils/utils.h"
#include <gsl/span>

namespace Halley {
    class UUID {
    public:
        UUID();
        UUID(gsl::span<Byte, 16> bytes);
        explicit UUID(const String& str);

        bool operator==(const UUID& other) const;
        bool operator!=(const UUID& other) const;
		bool operator<(const UUID& other) const;

		String toString() const;

		static UUID generate();
    	bool isValid() const;

    private:
		Byte bytes[16];
    };
}
