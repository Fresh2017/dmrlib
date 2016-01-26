#include <errno.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <talloc.h>
#include "dmr/error.h"
#include "dmr/log.h"
#include "dmr/payload/data.h"
#include "dmr/proto/sms.h"

static char *default_inbox = "";
static char *default_outbox = "outbox";
static char *maildirs[] = {"cur", "new", "tmp", NULL};

static int maildirmake(char *path)
{
	int ret = 0, i;
	char wd[PATH_MAX];
	if (getcwd(wd, PATH_MAX) == NULL) {
		dmr_log_error("sms: can't get current working directory: %s", strerror(errno));
		return ret;
	}
	ret = mkdir(path, 0700);
	if (ret != 0 && errno != EEXIST) {
		dmr_log_error("sms: mkdir \"%s\": %s", path, strerror(errno));
		return ret;
	}
	if ((ret = chdir(path)) != 0) {
		dmr_log_error("sms: chdir \"%s\": %s", path, strerror(errno));
		return ret;
	}
	for (i = 0; maildirs[i] != NULL; i++) {
		ret = mkdir(maildirs[i], 0700);
		if (ret != 0 && errno != EEXIST) {
			dmr_log_error("sms: mkdir \"%s/%s\": %s", path, maildirs[i], strerror(errno));
			chdir(wd);
			return ret;
		}
		ret = 0;
	}
	if ((ret = chdir(wd)) != 0) {
		dmr_log_error("sms: chdir(%s): %s", wd, strerror(errno));
		return ret;
	}
	return ret;
}

static int joinpath(char *dst, char *dir, char *file)
{
	char abspath[PATH_MAX];
	if (realpath(dir, abspath) == NULL) {
		dmr_error_set("error resolving path %s: %s", dst, strerror(errno));
		return -1;
	}
	if (strlen(file) > 0) {
		snprintf(dst, PATH_MAX, "%s/%s", abspath, file);
	} else {
		strncpy(dst, abspath, PATH_MAX);
	}
	return 0;
}

static int sms_proto_init(void *smsptr)
{
	dmr_log_trace("sms: init %p", smsptr);
    if (smsptr == NULL)
        return dmr_error(DMR_EINVAL);
    return 0;
}

static int sms_proto_start(void *smsptr)
{
	dmr_log_trace("sms: start %p", smsptr);
    if (smsptr == NULL)
        return dmr_error(DMR_EINVAL);
    return 0;
}

static void sms_proto_tx(void *smsptr, dmr_packet_t *packet)
{
    dmr_log_trace("sms: tx %p", smsptr);
    dmr_sms_t *sms = (dmr_sms_t *)smsptr;

    if (sms == NULL || packet == NULL)
        return;

    dmr_ts_t ts = packet->ts;
    switch (packet->data_type) {
    case DMR_DATA_TYPE_DATA_HEADER: {
    	// Parse data header.
    	if (dmr_data_header_decode(sms->ts[ts].header, packet) != 0) {
    		dmr_log_error("sms: failed to decode data header: %s", dmr_error_get());
    		return;
    	}
    	dmr_log_debug("sms: data header: %s, service access point: %s",
    		dmr_dpf_name(sms->ts[ts].header->dpf),
    		dmr_sap_name(sms->ts[ts].header->sap));

    	switch (sms->ts[ts].header->dpf) {
    		case DMR_DPF_DEFINED_SHORT: {
    			if (sms->ts[ts].header->data.defined_short.full_message) {
	    			memset(sms->ts[ts].blocks, 0, sizeof(sms->ts[ts].blocks));
	    			sms->ts[ts].blocks_expected = sms->ts[ts].header->data.defined_short.appended_blocks;
	    		}
	    		sms->ts[ts].blocks_full = sms->ts[ts].header->data.defined_short.appended_blocks;
	    		dmr_log_debug("sms: expecting %u data blocks", sms->ts[ts].blocks_full);
    			break;
    		}
	    	case DMR_DPF_RAW_SHORT: {
	    		if (sms->ts[ts].header->data.raw_short.full_message) {
	    			memset(sms->ts[ts].blocks, 0, sizeof(sms->ts[ts].blocks));
	    			sms->ts[ts].blocks_expected = sms->ts[ts].header->data.raw_short.appended_blocks;
	    		}
	    		sms->ts[ts].blocks_full = sms->ts[ts].header->data.raw_short.appended_blocks;
	    		dmr_log_debug("sms: expecting %u data blocks", sms->ts[ts].blocks_full);
	    		break;
	    	}
	    	default:
	    		break;
    	}    		
    	break;
    }
    case DMR_DATA_TYPE_RATE34_DATA: {
    	dmr_data_block_t block;
    	if (dmr_data_block_decode(&block, sms->ts[ts].header->response_requested, packet) != 0) {
    		dmr_log_error("sms: failed to decode data block: %s", dmr_error_get());
    		return;
    	}
    	break;
    }
    default:
    	break;
    }
}

dmr_sms_t *dmr_sms_new(char *path, char *inbox, char *outbox)
{
	if (path == NULL) {
		dmr_log_error("sms: path can't be NULL");
		return NULL;
	}

	dmr_sms_t *sms = talloc_zero(NULL, dmr_sms_t);

	// Setup buffers
	dmr_ts_t ts;
	for (ts = DMR_TS1; ts < DMR_TS_INVALID; ts++) {
		sms->ts[ts].header = talloc_zero(sms, dmr_data_header_t);
		if (sms->ts[ts].header == NULL) {
			dmr_log_error("sms: out of memory");
			TALLOC_FREE(sms);
			return NULL;
		}
	}

	// Setup spool
	sms->path = talloc_strdup(sms, path);
	sms->inbox = talloc_size(sms, PATH_MAX);
	sms->outbox = talloc_size(sms, PATH_MAX);
	if (sms->path == NULL || sms->inbox == NULL || sms->outbox == NULL) {
		dmr_log_critical("sms: out of memory");
		TALLOC_FREE(sms);
		return NULL;
	}
	if (joinpath(sms->inbox, sms->path, inbox == NULL ? default_inbox : inbox) != 0) {
		dmr_log_critical("sms: failed to resolve inbox: %s", dmr_error_get());
		TALLOC_FREE(sms);
		return NULL;
	}
	if (joinpath(sms->outbox, sms->path, outbox == NULL ? default_outbox : outbox) != 0) {
		dmr_log_critical("sms: failed to resolve outbox: %s", dmr_error_get());
		TALLOC_FREE(sms);
		return NULL;
	}
	if (maildirmake(sms->inbox) != 0) {
		dmr_log_critical("sms: failed to create inbox: %s", sms->inbox);
		TALLOC_FREE(sms);
		return NULL;
	}
	if (maildirmake(sms->outbox) != 0) {
		dmr_log_critical("sms: failed to create outbox: %s", sms->outbox);
		TALLOC_FREE(sms);
		return NULL;
	}

	// Setup protocol
    sms->proto.name = "sms";
    sms->proto.type = DMR_PROTO_SMS;
    sms->proto.init = sms_proto_init;
    sms->proto.start = sms_proto_start;
    //mbe->proto.process_voice = mbe_proto_process_voice;
    sms->proto.tx = sms_proto_tx;

	return sms;
}