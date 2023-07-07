#include <math.h>

namespace trnr::lib::util {
	static inline double lin_2_db(double lin) {
		return 20 * log(lin);
	}

	static inline double db_2_lin(double db) {
		return pow(10, db/20);
	}
}