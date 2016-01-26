#include <dmr/payload/lc.h>
#include <dmr/payload/sync.h>
#include "_test_header.h"

static uint8_t raw[2][33] = {
	{
		0x06, 0x4a, 0x0f, 0xdc, 0x0e, 0xe4, 0x1e, 0x78, 0x6d, 0x70, 0x8e,
		0x01, 0x04, 0x6d, 0x5d, 0x7f, 0x77, 0xfd, 0x75, 0x7e, 0x32, 0x90,
		0x1d, 0x80, 0x18, 0x70, 0xad, 0xc0, 0x11, 0x00, 0x3c, 0x81, 0x8a
	}, {
		0x82, 0xcd, 0x86, 0x63, 0x31, 0x91, 0xa8, 0x83, 0x7f, 0xb3, 0xec,
		0xa5, 0x03, 0x57, 0x55, 0xfd, 0x7d, 0xf7, 0x5f, 0x75, 0xf5, 0x98,
		0xb0, 0x4f, 0x92, 0xdf, 0x87, 0x23, 0x75, 0xb1, 0xfd, 0x82, 0x7f
	}
};

bool test_decode(void)
{
	dmr_packet_t *packet = talloc_zero(NULL, dmr_packet_t);
	dmr_full_lc_t *full_lc = talloc_zero(NULL, dmr_full_lc_t);
	uint8_t i;

	for (i = 0; i < 1; i++) {
		memcpy(packet->payload, raw[i], 33);
		dmr_dump_hex(packet->payload, 33);
		memset(full_lc, 0, sizeof(dmr_full_lc_t));

		eq(dmr_sync_pattern_decode(packet) != DMR_SYNC_PATTERN_BS_SOURCED_DATA, "sync pattern mismatch");
		packet->data_type = DMR_DATA_TYPE_VOICE_LC;
		go(dmr_full_lc_decode(full_lc, packet), "full LC decode");
		printf("full link control: flco_pdu=%u, privacy=%u, fid=%u, %u->%u\n",
			full_lc->flco_pdu, full_lc->privacy, full_lc->fid,
			full_lc->src_id, full_lc->dst_id);
	}

	talloc_free(packet);
	talloc_free(full_lc);

	return true;
}


bool test_encode(void)
{
	dmr_packet_t *packet = talloc_zero(NULL, dmr_packet_t);
	dmr_full_lc_t *full_lc = talloc_zero(NULL, dmr_full_lc_t);
	dmr_full_lc_t *full_lc_decoded = talloc_zero(NULL, dmr_full_lc_t);

	memset(packet, 0, sizeof(dmr_packet_t));
	packet->src_id = rand() & 0xffffff;
	packet->dst_id = rand() & 0xffffff;
	packet->repeater_id = rand();
	packet->data_type = DMR_DATA_TYPE_VOICE_LC;
	dmr_dump_hex(packet->payload, 33);

	go(dmr_sync_pattern_encode(DMR_SYNC_PATTERN_BS_SOURCED_DATA, packet), "sync pattern encode");
	dmr_dump_hex(packet->payload, 33);

	full_lc->src_id = packet->src_id;
	full_lc->dst_id = packet->dst_id;
	printf("full link control: flco_pdu=%u, privacy=%u, fid=%u, %u->%u\n",
			full_lc->flco_pdu, full_lc->privacy, full_lc->fid,
			full_lc->src_id, full_lc->dst_id);
	go(dmr_full_lc_encode(full_lc, packet), "encode");
	dmr_dump_hex(packet->payload, 33);

	go(dmr_full_lc_decode(full_lc_decoded, packet), "decode");
	printf("full link control: flco_pdu=%u, privacy=%u, fid=%u, %u->%u\n",
			full_lc_decoded->flco_pdu, full_lc_decoded->privacy, full_lc_decoded->fid,
			full_lc_decoded->src_id, full_lc_decoded->dst_id);

	talloc_free(packet);
	talloc_free(full_lc_decoded);
	talloc_free(full_lc);

	return true;
}

static test_t tests[] = {
    {"Voice LC decode", test_decode},
    {"Voice LC encode", test_encode},
    {NULL, NULL} /* sentinel */
};

#include "_test_footer.h"
