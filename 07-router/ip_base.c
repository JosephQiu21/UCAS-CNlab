#include "ip.h"
#include "icmp.h"
#include "arpcache.h"
#include "rtable.h"
#include "arp.h"

#include "log.h"

#include <stdio.h>
#include <stdlib.h>

// initialize ip header 
void ip_init_hdr(struct iphdr *ip, u32 saddr, u32 daddr, u16 len, u8 proto)
{
	ip->version = 4;
	ip->ihl = 5;
	ip->tos = 0;
	ip->tot_len = htons(len);
	ip->id = rand();
	ip->frag_off = htons(IP_DF);
	ip->ttl = DEFAULT_TTL;
	ip->protocol = proto;
	ip->saddr = htonl(saddr);
	ip->daddr = htonl(daddr);
	ip->checksum = ip_checksum(ip);
}

// lookup in the routing table, to find the entry with the same and longest prefix.
// the input address is in host byte order
rt_entry_t *longest_prefix_match(u32 dst)
{
	rt_entry_t *p = NULL;
	rt_entry_t *longest_match = NULL;
	u32 longest_prefix = 0;
	list_for_each_entry(p, &rtable, list) {
		if ((dst & p->mask) == (p->dest & p->mask)) {
			if (p->mask > longest_prefix) {
				longest_prefix = p->mask;
				longest_match = p;
			}
		}
	}
	return longest_match;
}

// send IP packet
//
// Different from forwarding packet, ip_send_packet sends packet generated by
// router itself. This function is used to send ICMP packets.
void ip_send_packet(char *packet, int len)
{
	struct iphdr *ip_hdr = packet_to_ip_hdr(packet);
	u32 daddr = ntohl(ip_hdr->daddr);
	rt_entry_t *entry = longest_prefix_match(daddr);
	if (entry == NULL) {
		log(ERROR, "no entry found in routing table");
	} else {
		ip_hdr->saddr = ntohl(entry->iface->ip);
		ip_hdr->checksum = ip_checksum(ip_hdr);
		iface_send_packet_by_arp(entry->iface, entry->gw == 0 ? daddr : entry->gw, packet, len);
	}
}
