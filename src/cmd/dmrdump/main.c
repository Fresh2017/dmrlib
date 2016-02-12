#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dmr/platform.h>

#include <pcap.h>
#include <pcap/pcap.h>
#if defined(DMR_PLATFORM_WINDOWS)
#include <winsock2.h>
#else
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#endif

#include <dmr.h>
#include <dmr/log.h>
#include <dmr/protocol/homebrew.h>
#include <dmr/fec.h>

#define ETHER_TYPE_IP (0x0800)

#include <sys/param.h>
/* Internet address.  */
typedef uint32_t in_addr_t;
#define ETH_ALEN       6               /* Octets in one ethernet addr   */
struct my_ether_header
{
    uint8_t  ether_dhost[ETH_ALEN];      /* destination eth addr */
    uint8_t  ether_shost[ETH_ALEN];      /* source ether addr    */
    uint16_t ether_type;                 /* packet type ID field */
} __attribute__ ((__packed__));
struct my_ip {
#if BYTE_ORDER == LITTLE_ENDIAN
    unsigned int ip_hl:4;               /* header length */
    unsigned int ip_v:4;                /* version */
#else
    unsigned int ip_v:4;                /* version */
    unsigned int ip_hl:4;               /* header length */
#endif
  	u_char	ip_tos;			// Type of service
  	u_short ip_len;			// Total length
  	u_short ip_id;      // Identification
  	u_short ip_off;		  // Flags (3 bits) + Fragment offset (13 bits)
  	u_char	ip_ttl;			// Time to live
  	u_char	ip_p;			  // Protocol
  	u_short ip_sum;			// Header checksum
  	struct in_addr ip_src, ip_dst;      /* source and dest address */
  	u_int	op_pad;			  // Option + Padding
};

struct my_udpheader {
	u_short uh_sport;			// Source port
	u_short uh_dport;			// Destination port
	u_short uh_len;			// Datagram length
	u_short uh_crc;			// Checksum
};

static struct option long_options[] = {
    {"source", required_argument, NULL, 'r'},
    {NULL, 0, NULL, 0} /* Sentinel */
};

void usage(const char *program)
{
    fprintf(stderr, "%s <args>\n\n", program);
    fprintf(stderr, "arguments:\n");
    fprintf(stderr, "\t-?, -h\t\t\tShow this help.\n");
    fprintf(stderr, "\t--source <source>\tPCAP source file.\n");
    fprintf(stderr, "\t-r <source>\n");
    fprintf(stderr, "\t-v\tIncrease verbosity.\n");
    fprintf(stderr, "\t-q\tDecrease verbosity.\n");
}

void dump_dmr_packet(dmr_parsed_packet *packet)
{
    dmr_dump_parsed_packet(packet);
    //dmr_sync_pattern sync_pattern;
    dmr_emb emb;

    switch (packet->data_type) {
    /*
    case DMR_DATA_TYPE_CSBK:
        dmr_payload_get_info_bits(&packet->payload, &info_bits);
        dmr_info_bits_deinterleave(&info_bits, &deinfo_bits);
        data_bits = dmrfec_bptc_196_96_decode(deinfo_bits.bits);
        if (data_bits == NULL) {
            fprintf(stderr, "csbk: BPTC(196, 96) extract failed\n");
            break;
        }
        csbk = dmr_csbk_decode(data_bits);
        if (csbk == NULL) {
            fprintf(stderr, "csbk: decode failed\n");
            break;
        }
        printf("csbk: %s, %u->%u, last %d\n",
            dmr_csbk_opcode_name(csbk->opcode),
            csbk->src_id, csbk->dst_id,
            csbk->last);
        break;
    */

    case DMR_DATA_TYPE_VOICE_LC:
    {
        dmr_full_lc full_lc;
        if (dmr_full_lc_decode(packet->packet, &full_lc, packet->data_type) != 0) {
            fprintf(stderr, "full LC decode failed: %s\n", dmr_error_get());
            return;
        }
        printf("full LC: flco=%s (%d), pf=%s, fid=%s (%d), %u->%u\n",
            dmr_flco_pdu_name(full_lc.flco_pdu), full_lc.flco_pdu,
            DMR_LOG_BOOL(full_lc.pf),
            dmr_fid_name(full_lc.fid), full_lc.fid,
            full_lc.src_id, full_lc.dst_id);
        dmr_dump_hex(&full_lc, sizeof(dmr_full_lc));
        dmr_packet debug;
        memcpy(&debug, packet->packet, sizeof(dmr_packet));
        if (dmr_full_lc_encode(debug, &full_lc, packet->data_type) != 0) {
            fprintf(stderr, "full LC encode failed: %s\n", dmr_error_get());
            return;
        }
        dmr_dump_packet(debug);
        printf("full LC: flco=%s (%d), pf=%s, fid=%s (%d), %u->%u\n",
            dmr_flco_pdu_name(full_lc.flco_pdu), full_lc.flco_pdu,
            DMR_LOG_BOOL(full_lc.pf),
            dmr_fid_name(full_lc.fid), full_lc.fid,
            full_lc.src_id, full_lc.dst_id);
        break;
    }
    case DMR_DATA_TYPE_VOICE_SYNC:
    {
        break;
    }
    case DMR_DATA_TYPE_VOICE:
    {
        // Frame should contain embedded signalling.
        if (dmr_emb_decode(packet->packet, &emb) != 0) {
            fprintf(stderr, "embedded signalling decode failed: %s\n", dmr_error_get());
            return;
        }
        printf("embedded signalling: color code %d, lcss %d (%s)",
            emb.color_code,
            emb.lcss, dmr_emb_lcss_name(emb.lcss));

        switch (emb.lcss) {
        case DMR_EMB_LCSS_SINGLE_FRAGMENT:
        {
            printf(", single fragment");
            uint8_t bytes[4];
            if (dmr_emb_bytes_decode(packet->packet, bytes) != 0) {
                printf(", bytes decode failed: %s\n", dmr_error_get());
                return;
            }
            if (dmr_emb_null(bytes)) {
                printf(", NULL EMB\n");
            } else {
                printf(", RC info\n");
            }
            break;
        }
        case DMR_EMB_LCSS_FIRST_FRAGMENT:
            printf(", first fragment\n");
            break;
        case DMR_EMB_LCSS_CONTINUATION:
            printf(", continuation\n");
            break;
        case DMR_EMB_LCSS_LAST_FRAGMENT:
            printf(", last fragment\n");
            break;
        default:
            break;
        }

        break;
    }
    default:
        break;
    }
}

void dump_homebrew(struct my_ip *ip_hdr, struct my_udpheader *udp, uint8_t *bytes, unsigned int len)
{
    dmr_homebrew homebrew;
    dmr_raw raw;
    raw.buf = bytes;
    raw.len = len;

    printf("%s:%d->%s:%d (%u bytes), ",
        inet_ntoa(ip_hdr->ip_src), ntohs(udp->uh_sport),
        inet_ntoa(ip_hdr->ip_dst), ntohs(udp->uh_dport),
        len);

    dmr_parsed_packet *packet = NULL;
    if (dmr_homebrew_parse_dmrd(&homebrew, &raw, &packet) != 0) {
        fprintf(stderr, "failed to parse frame: %s", dmr_error_get());
        return;
    }
    if (packet == NULL)
        return;

    dump_dmr_packet(packet);
}

int main(int argc, char **argv)
{
    int ch;
    const char *source = NULL;
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;
    struct pcap_pkthdr header;
    const uint8_t *packet;

    while ((ch = getopt_long(argc, argv, "r:h?vq", long_options, NULL)) != -1) {
        switch (ch) {
        case -1:       /* no more arguments */
        case 0:        /* long options toggles */
            break;
        case 'h':
        case '?':
            usage(argv[0]);
            return 0;
        case 'r':
            source = strdup(optarg);
            break;
        case 'v':
            dmr_log_priority_set(dmr_log_priority() - 1);
            break;
        case 'q':
            dmr_log_priority_set(dmr_log_priority() + 1);
            break;
        default:
            return 1;
        }
    }

    if (source == NULL) {
        usage(argv[0]);
        return 1;
    }

    if (dmr_fec_init() != 0) {
        fprintf(stderr, "FEC init failed\n");
        return 1;
    }

    if ((handle = pcap_open_offline(source, errbuf)) == NULL) {
        fprintf(stderr, "error opening %s: %s\n", source, errbuf);
        return 1;
    }

    size_t packets = 0;
    while ((packet = pcap_next(handle, &header)) != NULL) {
        packets++;
        if (header.caplen < 14) {
            fprintf(stderr, "skipping non-ethernet frame\n");
            continue; // No an Ethernet frame
        }
        unsigned int caplen = header.caplen;
        uint8_t *pkt_ptr = (uint8_t *)packet;
        int ether_type = ((int)(pkt_ptr[12]) << 8) | (int)pkt_ptr[13];
        if (ether_type != ETHER_TYPE_IP) {
            fprintf(stderr, "unknown ethernet type %04X, skipping...\n", ether_type);
        }

        pkt_ptr += sizeof(struct my_ether_header);
        caplen -= sizeof(struct my_ether_header);
        if (caplen < sizeof(struct my_ip))
            continue;

        struct my_ip *ip_hdr = (struct my_ip *)pkt_ptr;
        unsigned int ip_headerlen = ip_hdr->ip_hl * 4;  /* ip_hl is in 4-byte words */
        if (caplen < ip_headerlen)
            continue;

        if (ip_hdr->ip_v != 4 || (ip_hdr->ip_p != 17 && ip_hdr->ip_p != 23)) {
            fprintf(stderr, "skipping packet IP version %d, protocol %d\n", ip_hdr->ip_v, ip_hdr->ip_p);
            continue;
        }

        pkt_ptr += ip_headerlen;
        caplen -= ip_headerlen;
        if (caplen < sizeof(struct my_udpheader))
            continue;

        struct my_udpheader *udp = (struct my_udpheader *)pkt_ptr;
        pkt_ptr += sizeof(struct my_udpheader);
        caplen -= sizeof(struct my_udpheader);
        if (ntohs(udp->uh_sport) == 62030 || ntohs(udp->uh_dport) == 62030) {
            dump_homebrew(ip_hdr, udp, pkt_ptr, caplen);
        }
    }

    pcap_close(handle);
    fprintf(stderr, "processed %zu packets\n", packets);
    return 0;
}
