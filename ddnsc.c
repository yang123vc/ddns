/* $Id: ddnsc.c,v 1.11 2000/07/14 23:57:56 drt Exp $
 *
 * client for ddns
 * 
 * $Log: ddnsc.c,v $
 * Revision 1.11  2000/07/14 23:57:56  drt
 * Added documentation in README and manpage for
 * ddns-clientd, rewriting of 0.0.0.1 in ddns-cliend
 * fixes in ddnsc.c and ddns-client.c
 *
 * Revision 1.10  2000/07/12 11:34:25  drt
 * ddns-clientd handels now everything itself.
 * ddnsc is now linked to ddnsd-clientd, do a
 * enduser needs just this single executable
 * and no ucspi-tcp/tcpclient.
 *
 * Revision 1.9  2000/07/07 13:32:47  drt
 * ddnsd and ddnsc now basically work as they should and are in
 * a usable state. The protocol is changed a little bit to lessen
 * problems with alignmed and so on.
 * Tested on IA32 and PPC.
 * Both are still missing support for LOC.
 *
 * Revision 1.8  2000/05/02 22:53:49  drt
 * Changed keysize to 256 bits
 * cleand code a bit and removed a lot of cruft
 * some checks for errors
 *
 * Revision 1.7  2000/04/30 23:03:18  drt
 * Unknown user is now communicated by using uid == 0
 *
 * Revision 1.6  2000/04/30 15:59:26  drt
 * cleand up usage of djb stuff
 *
 * Revision 1.5  2000/04/30 14:56:57  drt
 * cleand up usage of djb stuff
 *
 * Revision 1.4  2000/04/27 12:12:40  drt
 * Changed data packets to 32+512 Bits size, added functionality to
 * transport IPv6 adresses and LOC records.
 *
 * Revision 1.3  2000/04/24 16:30:56  drt
 * First basically working version implementing full protocol
 *
 * Revision 1.2  2000/04/21 06:58:36  drt
 * *** empty log message ***
 *
 * Revision 1.1.1.1  2000/04/17 16:18:35  drt
 * initial ddns version
 *
 */

#include "buffer.h"
#include "byte.h"
#include "env.h"
#include "error.h"
#include "fmt.h"
#include "ip4.h"
#include "scan.h"
#include "stralloc.h"
#include "strerr.h"
#include "timeoutread.h"
#include "timeoutwrite.h"

#include "mt19937.h"
#include "rijndael.h"
#include "loc.h"

#include "ddns.h"

static char rcsid[] = "$Id: ddnsc.c,v 1.11 2000/07/14 23:57:56 drt Exp $";

#define FATAL "ddns-client: "

/* read, decode and decrypt packet */
int ddnsc_recive(struct ddnsreply *p)
{
  int r;
  struct ddnsreply ptmp = {0};

  r = timeoutread(120, 6, &ptmp, sizeof(struct ddnsreply));
  if(r != 68)
    strerr_die2x(100, FATAL, "wrong packetsize");
  
  uint32_unpack((char*) &ptmp.uid, &p->uid);
  
  /* XXX: check for my own userid */
  
  /* the key should be set up by ddnsc_send already */
  
  /* decrypt with rijndael */
  rijndaelDecrypt((char *) &ptmp.type);
  rijndaelDecrypt((char *) &ptmp.type + 32);

  uint16_unpack((char*) &ptmp.type, &p->type);
  uint32_unpack((char*) &ptmp.magic, &p->magic);
  taia_unpack((char*) &ptmp.timestamp, &p->timestamp);
  uint32_unpack((char*) &ptmp.leasetime, &p->leasetime);

  if(p->uid == 0)
    strerr_die2x(100, FATAL, "user unknown to server"); 

  if(p->magic != DDNS_MAGIC)
    strerr_die2x(100, FATAL, "wrong magic");
  
  return p->type;
}

/* fill p with random, timestamp, magic, encrypt it and send it */
static void ddnsc_send(struct ddnsrequest *p, char *key)
{
  struct taia t;
  struct ddnsrequest ptmp = {0};

  /* fill our structure */
  /* and get it into network byte order */
  p->magic = DDNS_MAGIC;
  taia_now(&t);
  taia_pack((char *) &ptmp.timestamp, &t);
  uint32_pack((char*) &ptmp.uid, p->uid);
  uint32_pack((char*) &ptmp.magic, p->magic);
  byte_copy(&ptmp.ip4[0], 4, p->ip4);
  byte_copy(&ptmp.ip6[0], 16, p->ip6);
  uint16_pack((char*) &ptmp.type, p->type);
  uint32_pack((char*) &ptmp.loc_lat, p->loc_lat);
  uint32_pack((char*) &ptmp.loc_long, p->loc_long);
  uint32_pack((char*) &ptmp.loc_alt, p->loc_alt);
  ptmp.loc_size = p->loc_size;
  ptmp.loc_hpre = p->loc_hpre;
  ptmp.loc_vpre = p->loc_vpre;
  ptmp.random1 = ptmp.random2 = randomMT() & 0xffff;
  /* fill reserved with random data */
  ptmp.reserved1 = randomMT() & 0xff;;
  ptmp.reserved2 = randomMT() & 0xffff;

  /* initialize rijndael with 256 bit blocksize and 256 bit keysize */
  rijndaelKeySched(8, 8, key);
    
  /* Encrypt with rijndael */
  rijndaelEncrypt((char *) &ptmp.type);
  rijndaelEncrypt((char *) &ptmp.type + 32);  

  if(timeoutwrite(60, 7, &ptmp, sizeof(struct ddnsrequest)) != 68)
    strerr_die2sys(100, FATAL, "couldn't write to network: ");
}

/* handle client <-> server communication */
int ddnsc(int type, uint32 uid, char *ip4, char *ip6, struct loc_s *loc, char *key, uint32 *ttl)
{
  struct ddnsrequest p = {0};
  struct ddnsreply r = {0};
  
  p.type = type;
  p.uid = uid;
  byte_copy(&p.ip4[0], sizeof(p.ip4), ip4);
  byte_copy(&p.ip6[0], sizeof(p.ip4), ip6);
  p.loc_lat = loc->latitude;
  p.loc_long = loc->longitude;
  p.loc_alt = loc->altitude;
  p.loc_size = loc->size;
  p.loc_hpre = loc->hpre;
  p.loc_vpre = loc->vpre;
  
  /* send request */
  ddnsc_send(&p, key);

  /* get answer */
  ddnsc_recive(&r);

  *ttl = r.leasetime;
  return r.type;
}
