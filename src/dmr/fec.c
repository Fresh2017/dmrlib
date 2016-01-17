#include "dmr/error.h"
#include "dmr/fec.h"

int dmr_fec_init(void)
{
	if (dmr_rs_init() != 0) {
		return dmr_error(DMR_LASTERROR);
	}
	return 0;
}