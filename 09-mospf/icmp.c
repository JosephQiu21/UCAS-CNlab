#include "icmp.h"
#include "ip.h"
#include "rtable.h"
#include "arp.h"
#include "base.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>

// send icmp packet
void icmp_send_packet(const char *in_pkt, int len, u8 type, u8 code)
{
	struct iphdr *in_ip = packet_to_ip_hdr(in_pkt);
	struct icmphdr *in_icmp = (struct icmphdr *)(IP_DATA(in_ip));

	int icmp_len = type == ICMP_ECHOREPLY ? ntohs(in_ip->tot_len) - IP_HDR_SIZE(in_ip) : ICMP_HDR_SIZE + IP_HDR_SIZE(in_ip) + ICMP_COPIED_DATA_LEN;
	int len_out_pkt = type == ICMP_ECHOREPLY ? len : ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + icmp_len;
	char *out_pkt = malloc(len_out_pkt);
	memset(out_pkt, 0, len_out_pkt);

	struct iphdr *out_ip = packet_to_ip_hdr(out_pkt);
	struct icmphdr *out_icmp = (struct icmphdr *)((char *)out_ip + IP_BASE_HDR_SIZE);
	out_icmp->type = type;
	out_icmp->code = code;

	if (type == ICMP_ECHOREPLY) {
		out_icmp->icmp_identifier = in_icmp->icmp_identifier;
		out_icmp->icmp_sequence = in_icmp->icmp_sequence;
		memcpy((char *)out_icmp + ICMP_HDR_SIZE, (char *)in_icmp + ICMP_HDR_SIZE, \
				icmp_len - ICMP_HDR_SIZE);
	} else {
		memcpy((char *)out_icmp + ICMP_HDR_SIZE, (char *)in_ip, icmp_len - ICMP_HDR_SIZE);
	}

	out_icmp->checksum = icmp_checksum(out_icmp, icmp_len);

	u32 saddr = 0, daddr = ntohl(in_ip->saddr);
	if (type == ICMP_ECHOREPLY) {
		saddr = ntohl(in_ip->daddr);
	} else {
		rt_entry_t *entry = longest_prefix_match(daddr);
		saddr = entry->iface->ip;
	}

	int out_ip_len = IP_BASE_HDR_SIZE + icmp_len;

	ip_init_hdr(out_ip, saddr, daddr, out_ip_len, IPPROTO_ICMP);

	ip_send_packet(out_pkt, len_out_pkt);
}
