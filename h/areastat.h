/* $Id$ */

typedef struct	{
unsigned short int	origNode,	/* 0 */
			destNode,	/* 2 */
			year,		/* 4	 origPoint for 2.2 */
			month,		/* 6	 destPoint for 2.2 */
			day,		/* 8 */
			hour,		/* A */
			minute, 	/* C */
			second, 	/* E */
			baud,		/* 10	 == 2 for 2.2	   */
			packettype,	/* 12 */
			origNet,	/* 14 */
			destNet;	/* 16 */
unsigned char		prodcode,	/* 18 */
			serialno;	/* 19 */
unsigned char		password[8];	/* 1A */
unsigned short int	origZone,	/* 22 */ 
			destZone,	/* 24 */
			auxNet, 	/* 26	 domains begin 2.2 */
			CapValid;	/* 28 */
unsigned char		prodcode2,	/* 2A */
			serialno2;	/* 2B */
unsigned short int	CapWord;	/* 2C */
unsigned short int	origZone2,	/* 2E */ 
			destZone2;	/* 30 */
unsigned short int	origPoint,	/* 32 */
			destPoint;	/* 34	 domains end  2.2 */
unsigned char		fill_36[4];	/* 36	  "XPKT" */
				/* 3A */
} PKT_hdr;

typedef struct	{
unsigned short int	origNode,
			destNode,
			origNet,
			destNet,
			AttributeWord,
			cost;
unsigned char		DateTime[20];
} MSG_hdr;
