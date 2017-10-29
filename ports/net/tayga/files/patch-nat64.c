--- nat64.c	2011-05-25 23:11:30.000000000 +0900
+++ nat64.c	2017-08-28 14:27:58.482123000 +0900
@@ -19,6 +19,13 @@
 
 extern struct config *gcfg;
 
+#if __BYTE_ORDER == __BIG_ENDIAN
+static uint16_t checksum_extend_byte(uint8_t b)
+{
+	return htons(b << 8);
+}
+#endif
+
 static uint16_t ip_checksum(void *d, int c)
 {
 	uint32_t sum = 0xffff;
@@ -30,7 +37,11 @@
 	}
 
 	if (c)
+#if __BYTE_ORDER == __BIG_ENDIAN
+		sum += checksum_extend_byte(*((uint8_t *)p));
+#else
 		sum += htons(*((uint8_t *)p) << 8);
+#endif
 
 	while (sum > 0xffff)
 		sum = (sum & 0xffff) + (sum >> 16);
@@ -94,8 +105,7 @@
 	} __attribute__ ((__packed__)) header;
 	struct iovec iov[2];
 
-	header.pi.flags = 0;
-	header.pi.proto = htons(ETH_P_IP);
+	TUN_SET_PROTO(&header.pi,  ETH_P_IP);
 	header.ip4.ver_ihl = 0x45;
 	header.ip4.tos = tos;
 	header.ip4.length = htons(sizeof(header.ip4) + sizeof(header.icmp) +
@@ -156,6 +166,7 @@
 	}
 }
 
+
 static void xlate_header_4to6(struct pkt *p, struct ip6 *ip6,
 		int payload_length)
 {
@@ -180,10 +191,20 @@
 		cksum = ones_add(p->icmp->cksum, cksum);
 		if (p->icmp->type == 8) {
 			p->icmp->type = 128;
+#if __BYTE_ORDER == __BIG_ENDIAN
+			p->icmp->cksum = ones_add(cksum,
+				~checksum_extend_byte(128 - 8));
+#else
 			p->icmp->cksum = ones_add(cksum, ~(128 - 8));
+#endif
 		} else {
 			p->icmp->type = 129;
+#if __BYTE_ORDER == __BIG_ENDIAN
+			p->icmp->cksum = ones_add(cksum,
+				~checksum_extend_byte(129 - 0));
+#else
 			p->icmp->cksum = ones_add(cksum, ~(129 - 0));
+#endif
 		}
 		return 0;
 	case 17:
@@ -266,8 +287,7 @@
 	if (dest)
 		dest->flags |= CACHE_F_SEEN_4TO6;
 
-	header.pi.flags = 0;
-	header.pi.proto = htons(ETH_P_IPV6);
+	TUN_SET_PROTO(&header.pi,  ETH_P_IPV6);
 
 	if (no_frag_hdr) {
 		iov[0].iov_base = &header;
@@ -514,8 +534,7 @@
 						sizeof(header.ip6_em)),
 				ip_checksum(p_em.data, p_em.data_len)));
 
-	header.pi.flags = 0;
-	header.pi.proto = htons(ETH_P_IPV6);
+	TUN_SET_PROTO(&header.pi,  ETH_P_IPV6);
 
 	iov[0].iov_base = &header;
 	iov[0].iov_len = sizeof(header);
@@ -566,8 +585,7 @@
 	} __attribute__ ((__packed__)) header;
 	struct iovec iov[2];
 
-	header.pi.flags = 0;
-	header.pi.proto = htons(ETH_P_IPV6);
+	TUN_SET_PROTO(&header.pi,  ETH_P_IPV6);
 	header.ip6.ver_tc_fl = htonl((0x6 << 28) | (tc << 20));
 	header.ip6.payload_length = htons(sizeof(header.icmp) + data_len);
 	header.ip6.next_header = 58;
@@ -588,6 +606,8 @@
 	if (writev(gcfg->tun_fd, iov, data_len ? 2 : 1) < 0)
 		slog(LOG_WARNING, "error writing packet to tun device: %s\n",
 				strerror(errno));
+
+	slog(LOG_WARNING, "Wrote somethinh\n");
 }
 
 static void host_send_icmp6_error(uint8_t type, uint8_t code, uint32_t word,
@@ -668,10 +688,20 @@
 		cksum = ones_add(p->icmp->cksum, cksum);
 		if (p->icmp->type == 128) {
 			p->icmp->type = 8;
+#if __BYTE_ORDER == __BIG_ENDIAN
+			p->icmp->cksum = ones_add(cksum,
+				checksum_extend_byte(128 - 8));
+#else
 			p->icmp->cksum = ones_add(cksum, 128 - 8);
+#endif
 		} else {
 			p->icmp->type = 0;
+#if __BYTE_ORDER == __BIG_ENDIAN
+			p->icmp->cksum = ones_add(cksum,
+				checksum_extend_byte(129 - 0));
+#else
 			p->icmp->cksum = ones_add(cksum, 129 - 0);
+#endif
 		}
 		return 0;
 	case 17:
@@ -728,8 +758,7 @@
 	if (dest)
 		dest->flags |= CACHE_F_SEEN_6TO4;
 
-	header.pi.flags = 0;
-	header.pi.proto = htons(ETH_P_IP);
+	TUN_SET_PROTO(&header.pi, ETH_P_IP);
 
 	header.ip4.cksum = ip_checksum(&header.ip4, sizeof(header.ip4));
 
@@ -932,8 +961,7 @@
 							sizeof(header.ip4_em)),
 				ip_checksum(p_em.data, p_em.data_len));
 
-	header.pi.flags = 0;
-	header.pi.proto = htons(ETH_P_IP);
+	TUN_SET_PROTO(&header.pi, ETH_P_IP);
 
 	iov[0].iov_base = &header;
 	iov[0].iov_len = sizeof(header);
