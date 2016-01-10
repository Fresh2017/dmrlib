#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dmr/platform.h>

#include <pcap.h>
#if defined(DMR_PLATFORM_WINDOWS)
#include <winsock2.h>
#else
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#endif

#include <dmr.h>
#include <dmr/proto/homebrew.h>
#include <dmrfec.h>

#define ETHER_TYPE_IP (0x0800)

#if defined(DMR_PLATFORM_WINDOWS)
#include <sys/param.h>
/* Internet address.  */
typedef uint32_t in_addr_t;
#define ETH_ALEN       6               /* Octets in one ethernet addr   */
struct ether_header
{
    u_int8_t  ether_dhost[ETH_ALEN];      /* destination eth addr */
    u_int8_t  ether_shost[ETH_ALEN];      /* source ether addr    */
    u_int16_t ether_type;                 /* packet type ID field */
} __attribute__ ((__packed__));
struct ip {
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
struct udphdr {
	u_short uh_sport;			// Source port
	u_short uh_dport;			// Destination port
	u_short uh_len;			// Datagram length
	u_short uh_crc;			// Checksum
};
#endif

static struct option long_options[] = {
    {"source", required_argument, NULL, 'r'},
    {NULL, 0, NULL, 0} /* Sentinel */
};

void init(void)
{
    fprintf(stderr, "initializing dmrfec\n");
    dmrfec_init();
}

void usage(const char *program)
{
    fprintf(stderr, "%s <args>\n\n", program);
    fprintf(stderr, "arguments:\n");
    fprintf(stderr, "\t-?, -h\t\t\tShow this help.\n");
    fprintf(stderr, "\t--source <source>\tPCAP source file.\n");
    fprintf(stderr, "\t-r <source>\n");
}

void dump_dmr_packet(dmr_packet_t *packet)
{
    dmr_payload_info_bits_t *info_bits;
    dmr_payload_sync_pattern_t sync_pattern;
    dmr_payload_sync_bits_t *sync_bits;
    dmrfec_bptc_196_96_data_bits_t *data_bits;
    dmr_csbk_t *csbk;

    printf("[ts%d][%u->%u(%u)] %s\n",
        packet->ts + 1,
        packet->src_id, packet->dst_id,
        packet->repeater_id,
        dmr_packet_get_slot_type_name(packet->slot_type));

    switch (packet->slot_type) {
    case DMR_SLOT_TYPE_CSBK:
        info_bits = dmr_payload_get_info_bits(&packet->payload);
        info_bits = dmr_info_bits_deinterleave(info_bits);
        data_bits = dmrfec_bptc_196_96_extractdata(info_bits->bits);
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

    case DMR_SLOT_TYPE_VOICE_LC:
        sync_bits = dmr_payload_get_sync_bits(&packet->payload);
        sync_pattern = dmr_sync_bits_get_sync_pattern(sync_bits);
        printf("sync pattern: %s\n", dmr_payload_get_sync_pattern_name(sync_pattern));
        break;

    case DMR_SLOT_TYPE_VOICE_BURST_A:
    case DMR_SLOT_TYPE_VOICE_BURST_B:
    case DMR_SLOT_TYPE_VOICE_BURST_C:
    case DMR_SLOT_TYPE_VOICE_BURST_D:
    case DMR_SLOT_TYPE_VOICE_BURST_E:
    case DMR_SLOT_TYPE_VOICE_BURST_F:
        sync_bits = dmr_payload_get_sync_bits(&packet->payload);
        sync_pattern = dmr_sync_bits_get_sync_pattern(sync_bits);
        if (sync_pattern != DMR_PAYLOAD_SYNC_PATTERN_UNKNOWN) {
            printf("sync pattern: %s\n", dmr_payload_get_sync_pattern_name(sync_pattern));
        } else {
            // Frame should contain embedded signalling.
            dmr_emb_t *emb = dmr_emb_decode(dmr_emb_from_payload_sync_bits(sync_bits));
            if (emb == NULL) {
                fprintf(stderr, "embedded signalling decode failed\n");
                return;
            }
            printf("embedded signalling: color code %d, lcss %d (%s)\n",
                emb->color_code, emb->lcss, dmr_emb_lcss_name(emb->lcss));

            if (emb->lcss == DMR_EMB_LCSS_SINGLE_FRAGMENT) {

            }
        }
        break;
    }
}

void dump_homebrew(struct ip *ip_hdr, struct udphdr *udp, const uint8_t *bytes, unsigned int len)
{
    dmr_homebrew_packet_t *packet;
    dmr_homebrew_frame_type_t frame_type = dmr_homebrew_frame_type(bytes, len);

    printf("%s:%d->%s:%d, %d bytes homebrew proto",
        inet_ntoa(ip_hdr->ip_src), ntohs(udp->uh_sport),
        inet_ntoa(ip_hdr->ip_dst), ntohs(udp->uh_dport),
        len);

    switch (frame_type) {
    case DMR_HOMEBREW_DMR_DATA_FRAME:
        printf(", DMR data frame\n");
        packet = dmr_homebrew_parse_packet(bytes, len);
        if (packet == NULL)
            return;

        dump_dmr_packet(&packet->dmr_packet);
        free(packet);
        break;
    default:
        printf(", unknown frame\n");
        dump_hex((void *)bytes, len);
        return;
    }
}

int main(int argc, char **argv)
{
    int ch;
    const char *source = NULL;
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;
    struct pcap_pkthdr header;
    const uint8_t *packet;

    while ((ch = getopt_long(argc, argv, "r:h?", long_options, NULL)) != -1) {
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
        default:
            return 1;
        }
    }

    if (source == NULL) {
        usage(argv[0]);
        return 1;
    }

    if ((handle = pcap_open_offline(source, errbuf)) == NULL) {
        fprintf(stderr, "error opening %s: %s\n", source, errbuf);
        return 1;
    }

    init();

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

        pkt_ptr += sizeof(struct ether_header);
        caplen -= sizeof(struct ether_header);
        if (caplen < sizeof(struct ip))
            continue;

        struct ip *ip_hdr = (struct ip *)pkt_ptr;
        unsigned int ip_headerlen = ip_hdr->ip_hl * 4;  /* ip_hl is in 4-byte words */
        if (caplen < ip_headerlen)
            continue;

        if (ip_hdr->ip_v != 4 || ip_hdr->ip_p != 23) {
            fprintf(stderr, "skipping packet IP version %d, protocol %d\n", ip_hdr->ip_v, ip_hdr->ip_p);
            continue;
        }

        pkt_ptr += ip_headerlen;
        caplen -= ip_headerlen;
        if (caplen < sizeof(struct udphdr))
            continue;

        struct udphdr *udp = (struct udphdr *)pkt_ptr;
        pkt_ptr += sizeof(struct udphdr);
        caplen -= sizeof(struct udphdr);
        if (ntohs(udp->uh_sport) == 62030 || ntohs(udp->uh_dport) == 62030) {
            dump_homebrew(ip_hdr, udp, pkt_ptr, caplen);
        }
    }

    pcap_close(handle);
    fprintf(stderr, "processed %zu packets\n", packets);
    return 0;
}
