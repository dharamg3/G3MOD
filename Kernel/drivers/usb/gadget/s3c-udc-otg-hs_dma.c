/*
 * drivers/usb/gadget/s3c-udc-otg-hs_dma.c
 * Samsung S3C on-chip full/high speed USB OTG 2.0 device controller dma mode
 *
 * Copyright (C) 2009 Samsung Electronics, Seung-Soo Yang
 * Copyright (C) 2008 Samsung Electronics, Kyu-Hyeok Jang, Seung-Soo Yang
 * Copyright (C) 2004 Mikko Lahteenmaki, Nordic ID
 * Copyright (C) 2004 Bo Henriksen, Nordic ID
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#if SERIAL_TRANSFER_DELAY
#define SERIAL_DELAY_uTIME		800
#endif

//extern void s3c_udc_soft_connect(void);
//extern void s3c_udc_soft_disconnect(void);
#ifdef CONFIG_USB_FILE_STORAGE
extern int MSC_INVALID_CBW_IGNORE_CLEAR_HALT(void);
#endif
static u8 clear_feature_num;
static int clear_feature_flag = 0;
static int zero_config_flag = 0;

bool tmp_1st = false;


//---------------------------------------------------------------------------------------

void s3c_show_mode(void)
{
	DEBUG("[S3C USB-OTG MODE] : DMA\n");
}
//---------------------------------------------------------------------------------------


/**
 * s3c_ep0_pre_setup
 * setup for receiving data stage of CONTROL SETUP transaction
 */
static inline void s3c_ep0_pre_setup(void)
{
	u32 ep_ctrl;

	DEBUG_SETUP("%s : Prepare Setup packets.\n", __func__);

	writel((1 << DEPTSIZ_PKT_CNT_BIT)|sizeof(struct usb_ctrlrequest), S3C_UDC_OTG_DOEPTSIZ(EP0_CON));	
	writel(virt_to_phys(&g_ctrl), S3C_UDC_OTG_DOEPDMA(EP0_CON));

	ep_ctrl = readl(S3C_UDC_OTG_DOEPCTL(EP0_CON));
	writel(ep_ctrl|DEPCTL_EPENA|DEPCTL_CNAK, S3C_UDC_OTG_DOEPCTL(EP0_CON));
}
//---------------------------------------------------------------------------------------

#if OTG_DEDICATED_FIFO
/**
 * s3c_ep_tx_fifo_flush
 * Flush the endpoint's Tx FIFO & Write the FIFO number to be used
 */
static inline u32 s3c_ep_tx_fifo_flush(u32 ctrl, u32 ep_num)
{
	/* Flush the endpoint's Tx FIFO */
	writel(ep_num<<TxFIFO_NUM_BIT_5, S3C_UDC_OTG_GRSTCTL);
	writel((ep_num<<TxFIFO_NUM_BIT_5)|(1<<TxFIFO_FLUSH_BIT), S3C_UDC_OTG_GRSTCTL);
	while(readl(S3C_UDC_OTG_GRSTCTL) & (1<<TxFIFO_FLUSH_BIT)) 	{ndelay(1);}

	/* Write the FIFO number to be used for this endpoint */
	ctrl &= ~(0xF << 22);
	ctrl |= (ep_num << 22);
	return ctrl;
}
//---------------------------------------------------------------------------------------
#endif

/**
 * s3c_ep_send_zlp
 * Send Zero Length Packet for ep_num
 */
static inline void s3c_ep_send_zlp(u32 ep_num)
{
	struct s3c_udc *dev = the_controller;
	u32 ctrl;
	
    if (ep_num == EP0_CON) {
        dev->ep0state = WAIT_FOR_SETUP;
        s3c_ep0_pre_setup();
    }
	ctrl = readl(S3C_UDC_OTG_DIEPCTL(ep_num));

#if OTG_DEDICATED_FIFO
	ctrl = s3c_ep_tx_fifo_flush(ctrl, ep_num);
#endif

	writel((1<<DEPTSIZ_PKT_CNT_BIT)|(0<<DEPTSIZ_XFER_SIZE_BIT), S3C_UDC_OTG_DIEPTSIZ(ep_num));
	writel(DEPCTL_EPENA|DEPCTL_CNAK|ctrl , S3C_UDC_OTG_DIEPCTL(ep_num));

}
//---------------------------------------------------------------------------------------

/**
 * s3c_udc_set_dma_rx
 * set rx dma to receive a request 
 */
static int s3c_udc_set_dma_rx(struct s3c_ep *ep, struct s3c_request *req)
{
	u32 *buf, ctrl;
	u32 length, pktcnt;
	u32 ep_num = ep_index(ep);
	
	buf = req->req.buf + req->req.actual;
	prefetchw(buf);

	length = req->req.length - req->req.actual;
	dma_cache_maint(buf, length, DMA_FROM_DEVICE);

	if(length == 0)
		pktcnt = 1;
	else
		pktcnt = (length - 1)/(ep->ep.maxpacket) + 1;

	switch(ep_num)
	{
		case EP0_CON: 		case EP1_OUT: 		case EP4_OUT: 			
		case EP7_OUT: 		case EP10_OUT: 		case EP13_OUT:			
			
			ctrl =  readl(S3C_UDC_OTG_DOEPCTL(ep_num));
			
			writel(virt_to_phys(buf), S3C_UDC_OTG_DOEPDMA(ep_num));
			writel((pktcnt<<DEPTSIZ_PKT_CNT_BIT)|(length<<DEPTSIZ_XFER_SIZE_BIT), S3C_UDC_OTG_DOEPTSIZ(ep_num));
			writel(DEPCTL_EPENA|DEPCTL_CNAK|ctrl, S3C_UDC_OTG_DOEPCTL(ep_num));
			
			DEBUG_OUT_EP("[%s] RX DMA start : DOEPDMA[%d] = 0x%x, DOEPTSIZ[%d] = 0x%x, DOEPCTL[%d] = 0x%x,\
				pktcnt = %d, xfersize = %d\n", __func__, \
				    ep_num, readl(S3C_UDC_OTG_DOEPDMA(ep_num)), 
				    ep_num, readl(S3C_UDC_OTG_DOEPTSIZ(ep_num)), 
				    ep_num, readl(S3C_UDC_OTG_DOEPCTL(ep_num)), 
					pktcnt, length);
			break;
		default:
			DEBUG_ERROR("[%s]: Error Unused EP[%d]\n", __func__, ep_num);
	}

	return 0;

}
//---------------------------------------------------------------------------------------

/**
 * s3c_udc_set_dma_tx
 * set tx dma to transmit a request 
 */
static int s3c_udc_set_dma_tx(struct s3c_ep *ep, struct s3c_request *req)
{
	u32 *buf, ctrl;
	u32 length, pktcnt;
	u32 ep_num = ep_index(ep); 
		
	buf = req->req.buf + req->req.actual;
	prefetch(buf);
	length = req->req.length - req->req.actual;
	
	if(ep_num == EP0_CON) 
		length = min(length, (u32)ep_maxpacket(ep));
	req->req.actual += length;
	dma_cache_maint(buf, length, DMA_TO_DEVICE);

	if(length == 0)
		pktcnt = 1;
	else
		pktcnt = (length - 1)/(ep->ep.maxpacket) + 1;

	ctrl = readl(S3C_UDC_OTG_DIEPCTL(ep_num));

#if OTG_DEDICATED_FIFO
	ctrl = s3c_ep_tx_fifo_flush(ctrl, ep_num);
#endif

	DEBUG_IN_EP("==[%s]: EP [%d]\n", __func__, ep_num);		

	switch(ep_num)
	{
		case EP0_CON:
		case EP2_IN:	case EP3_IN:
		case EP5_IN:	case EP6_IN:	
		case EP8_IN: 	case EP9_IN:	
		case EP11_IN:	case EP12_IN:			
		case EP14_IN:	case EP15_IN:			
						
			writel((pktcnt<<DEPTSIZ_PKT_CNT_BIT)|(length<<DEPTSIZ_XFER_SIZE_BIT), S3C_UDC_OTG_DIEPTSIZ(ep_num));
			writel(virt_to_phys(buf), S3C_UDC_OTG_DIEPDMA(ep_num)); 	
			writel(DEPCTL_EPENA|DEPCTL_CNAK|ctrl , S3C_UDC_OTG_DIEPCTL(ep_num));
			
			DEBUG_IN_EP("[%s] :TX DMA start : DIEPDMA[%d] = 0x%x, DIEPTSIZ[%d] = 0x%x, DIEPCTL[%d] = 0x%x,"
					 "pktcnt = %d, xfersize = %d\n", __func__, 
					ep_num, readl(S3C_UDC_OTG_DIEPDMA(ep_num)),
					ep_num, readl(S3C_UDC_OTG_DIEPTSIZ(ep_num)),
					ep_num, readl(S3C_UDC_OTG_DIEPCTL(ep_num)),
					pktcnt, length);

			break;
		default:
			DEBUG_ERROR("[%s]: Error Unused EP[%d]\n", __func__, ep_num);
	}
	return length;
}
//---------------------------------------------------------------------------------------

/**
 * handle_rx_complete
 * handle of completing rx 
 */
static void handle_rx_complete(struct s3c_udc *dev, u32 ep_num)
{
	struct s3c_ep *ep = &dev->ep[ep_num];
	struct s3c_request *req = NULL;
	u32 csr = 0, count_bytes=0, xfer_length, is_short = 0;
	DEBUG_OUT_EP("%s EP [%d]\n",__func__, ep_num);

	if (list_empty(&ep->queue)) 
	{
		DEBUG_OUT_EP("[%s] NULL REQ on OUT EP-%d\n", __func__, ep_num);
		return;
	}
	req = list_entry(ep->queue.next, struct s3c_request, queue);

	csr = readl(S3C_UDC_OTG_DOEPTSIZ(ep_num));
	switch(ep_num)
	{
		case EP0_CON: 
			count_bytes = (csr & 0x7f);
			break;
		case EP1_OUT: 
		case EP4_OUT: 			
		case EP7_OUT: 			
		case EP10_OUT: 	
		case EP13_OUT:			
			count_bytes = (csr & 0x7fff);
			break;
		default:
			DEBUG_ERROR("[%s]: Error Unused EP[%d]\n", __func__, ep_num);
	}

	dma_cache_maint(req->req.buf, req->req.length, DMA_FROM_DEVICE);
	xfer_length = req->req.length-count_bytes;
	req->req.actual += min(xfer_length, req->req.length-req->req.actual);
	is_short = (xfer_length < ep->ep.maxpacket);

	DEBUG_OUT_EP("[%s] RX DMA done : %d/%d bytes received%s, DOEPTSIZ = 0x%x, %d bytes remained\n",
			__func__, req->req.actual, req->req.length,
			is_short ? "/S" : "", csr, count_bytes);
	DEBUG_OUT_EP("%s : req->length = %d / req->actual = %d / xfer_length = %d / ep.maxpacket = %d / is_short = %d \n",
			__func__, req->req.length,req->req.actual,
			xfer_length,ep->ep.maxpacket, is_short);

	if (is_short || req->req.actual == xfer_length) 
	{
		if(ep_num == EP0_CON && dev->ep0state == DATA_STATE_RECV)
		{
			dev->ep0state = WAIT_FOR_SETUP;
			s3c_req_done(ep, req, 0);
			s3c_ep_send_zlp(EP0_CON);	
		}
		else
		{
			s3c_req_done(ep, req, 0);

			if(!list_empty(&ep->queue)) 
			{
				req = list_entry(ep->queue.next, struct s3c_request, queue);
				DEBUG_OUT_EP("[%s] Next Rx request start...\n", __func__);
				s3c_udc_set_dma_rx(ep, req);
			}
		}
	}
}
//---------------------------------------------------------------------------------------

/**
 * handle_tx_complete
 * handle of completing tx 
 */
static void handle_tx_complete(struct s3c_udc *dev, u32 ep_num)
{
	struct s3c_ep *ep = &dev->ep[ep_num];
	struct s3c_request *req;
	u32 count_bytes = 0;

	if (list_empty(&ep->queue)) 
	{
		DEBUG_IN_EP("[%s] : NULL REQ on IN EP-%d\n", __func__, ep_num);
		return;
	}

	req = list_entry(ep->queue.next, struct s3c_request, queue);

	if(ep_num == EP0_CON && dev->ep0state == DATA_STATE_XMIT)
	{
		u32 last = s3c_ep0_write_fifo(ep, req);
		if(last)
		{
			dev->ep0state = WAIT_FOR_SETUP;
		}
		return;
	}

	switch(ep_num)
	{
		case EP0_CON:
			count_bytes = (readl(S3C_UDC_OTG_DIEPTSIZ(ep_num))) & 0x7f;
			req->req.actual = req->req.length-count_bytes;
			DEBUG_IN_EP("[%s] : TX DMA done : %d/%d bytes sent, DIEPTSIZ0 = 0x%x\n",
					__func__, req->req.actual, req->req.length,
					readl(S3C_UDC_OTG_DIEPTSIZ(ep_num)));
			break;
			
		case EP2_IN:		case EP3_IN:
		case EP5_IN:		case EP6_IN:						
		case EP8_IN:		case EP9_IN:						
		case EP11_IN:		case EP12_IN:						
		case EP14_IN:		case EP15_IN:						
			
			count_bytes = (readl(S3C_UDC_OTG_DIEPTSIZ(ep_num))) & 0x7fff;
			req->req.actual = req->req.length-count_bytes;
			DEBUG_IN_EP("[%s] : TX DMA done : %d/%d bytes sent, DIEPTSIZ[%d] = 0x%x\n",
					__func__, req->req.actual, req->req.length,
					ep_num, readl(S3C_UDC_OTG_DIEPTSIZ(ep_num)));
			break;
			
		default:
			DEBUG_ERROR("[%s]: Error Unused EP[%d]\n", __func__, ep_num);
	}

	if (req->req.actual == req->req.length) 
	{
#if S3C_UDC_ZLP	
		if (req->zlp == true)
		{			
			DEBUG("[%s] S3C_UDC_ZLP req->req.actual == req->req.length\n", __func__);
			s3c_ep_send_zlp(ep_num);
			req->zlp = false;
			req->req.zero = false;
		}
#endif		
		s3c_req_done(ep, req, 0);

		if(!list_empty(&ep->queue)) 
		{
			req = list_entry(ep->queue.next, struct s3c_request, queue);
			DEBUG_IN_EP("[%s] : Next Tx request start...\n", __func__);
			s3c_udc_set_dma_tx(ep, req);
		}
	}
}
//---------------------------------------------------------------------------------------

/*
 *	s3c_req_done - retire a request; caller blocked irqs
 */
static void s3c_req_done(struct s3c_ep *ep, struct s3c_request *req, int status)
{
	u32 stopped = ep->stopped;

	DEBUG("[%s] %s %p, req = %p, stopped = %d\n",__func__, ep->ep.name, ep, &req->req, stopped);
	list_del_init(&req->queue);

	if (likely(req->req.status == -EINPROGRESS))
		req->req.status = status;
	else
		status = req->req.status;

	if (status && status != -ESHUTDOWN)
	{
		DEBUG("complete %s req %p stat %d len %u/%u\n",
			ep->ep.name, &req->req, status,
			req->req.actual, req->req.length);
	}

	/* don't modify queue heads during completion callback */
	ep->stopped = 1;

	S3C_UDC_UNLOCK(&ep->dev->lock);
	req->req.complete(&ep->ep, &req->req);
	S3C_UDC_LOCK(&ep->dev->lock);
	
	ep->stopped = stopped;
}
//---------------------------------------------------------------------------------------

/*
 * 	s3c_ep_nuke - dequeue ALL requests
 */
void s3c_ep_nuke(struct s3c_ep *ep, int status)
{
	struct s3c_request *req;

	DEBUG("[%s] %s %p\n", __func__, ep->ep.name, ep);

	/* called with irqs blocked */
	while (!list_empty(&ep->queue)) 
	{
		req = list_entry(ep->queue.next, struct s3c_request, queue);
		s3c_req_done(ep, req, status);
	}
}
//---------------------------------------------------------------------------------------

/**
 * s3c_udc_set_disconnect_state
 * make s3c-udc logically being disconnected
 * not physically disconnected
 * if pullup() doesn't power off UDC
 */
static void s3c_udc_set_disconnect_state(struct s3c_udc *dev)
{
	u8 i;
	
	dev->gadget.speed = USB_SPEED_UNKNOWN;

	//at reset
	if (dev->udc_state == USB_STATE_DEFAULT)
		return;
	
	/* prevent new request submissions, kill any outstanding requests  */
	for (i = 0; i <= S3C_MAX_ENDPOINTS; i++) 
	{
		struct s3c_ep *ep = &dev->ep[i];
		ep->stopped = 1;
		s3c_ep_nuke(ep, -ESHUTDOWN);
	}

	/* report disconnect; the driver is already quiesced */
	DEBUG_PM("disconnect, gadget %s\n", dev->driver->driver.name);
	if (dev->driver && dev->driver->disconnect) {
		S3C_UDC_UNLOCK(&dev->lock);
		dev->driver->disconnect(&dev->gadget);
		S3C_UDC_LOCK(&dev->lock);
	}

	/* re-init driver-visible data structures */
	s3c_ep_list_reinit(dev);

	
	tmp_1st = false;
}
//---------------------------------------------------------------------------------------

/**
 * s3c_udc_stop_activity
 * stop s3c-udc acting related with tx/rx and disconnect
 * Caller must hold lock for s3c_udc_set_disconnect_state()
 */
static void s3c_udc_stop_activity(struct s3c_udc *dev, struct usb_gadget_driver *driver)
{
	/* don't disconnect drivers more than once */
	if (dev->gadget.speed == USB_SPEED_UNKNOWN)
		driver = NULL;
	else
		s3c_udc_set_disconnect_state(dev);
}
//---------------------------------------------------------------------------------------

static void s3c_udc_reset(struct s3c_udc *dev)
{
	int i;
#if 0
		/*	only re-connect if not configured */
		if(dev->udc_state != USB_STATE_CONFIGURED)
		{
			// 3. Put the OTG device core in the disconnected state.
			s3c_udc_soft_disconnect();
			udelay(20);
	
			// 4. Make the OTG device core exit from the disconnected state.
			s3c_udc_soft_connect();
		}
#endif	
	s3c_udc_set_address(dev, 0);
	s3c_udc_set_disconnect_state(dev);

	/* 	 6. Unmask the core interrupts */
	writel(GINTMSK_RESET, S3C_UDC_OTG_GINTMSK);

	/* 	 7. Set NAK bit of EP */
	writel(DEPCTL_EPDIS|DEPCTL_SNAK, S3C_UDC_OTG_DOEPCTL(EP0_CON)); 
	udelay(10);

	/* 8. Unmask EP interrupts on IN  EPs : 0, 2, 3 */
	/* 		         		      OUT EPs : 0, 1 */
	writel( (((1<<EP0_CON))<<16) |(1<<EP0_CON), S3C_UDC_OTG_DAINTMSK);

	/* 9. Unmask device OUT EP common interrupts */
	writel(DOEPMSK_INIT, S3C_UDC_OTG_DOEPMSK);

	/* 10. Unmask device IN EP common interrupts */
	writel(DIEPMSK_INIT, S3C_UDC_OTG_DIEPMSK);

#define RX_FIFO_SIZE				(MAX_RX_FIFO_SIZE>>2)
#define NPTX_FIFO_START_ADDR		RX_FIFO_SIZE
#define NPTX_FIFO_SIZE				(MAX_NPTX_FIFO_SIZE>>2)
#define PTX_FIFO_SIZE				(MAX_PTX_FIFO_SIZE>>2)

	/* 11. Set Rx FIFO Size */
	writel(RX_FIFO_SIZE, S3C_UDC_OTG_GRXFSIZ);

	/* 
	 *	Set IN Endpoint TxFIFO 0 size & start address
	 *	(For Device mode) (16 ~ 7936)
 	 */
	writel(NPTX_FIFO_SIZE<<16| NPTX_FIFO_START_ADDR, S3C_UDC_OTG_GNPTXFSIZ);

	/* 13. Clear NAK bit of EP0, EP1, EP2 */
	//writel(DEPCTL_EPDIS|DEPCTL_CNAK, S3C_UDC_OTG_DOEPCTL(EP0_CON)); /* EP0: Control OUT */

	/* 
	 *	Set periodic TxFIFO size & start address
	 *	(For Device mode) (4 ~ 768)
 	 */
	for (i = 1; i <= S3C_MAX_ENDPOINTS; i++)
		writel(PTX_FIFO_SIZE << 16 |
		(NPTX_FIFO_START_ADDR + NPTX_FIFO_SIZE + PTX_FIFO_SIZE*(i-1)), S3C_UDC_OTG_DIEPTXFSIZ(i));

	/* Flush the RX FIFO */
	//0x10 all fifo, 0 ~ f
	writel(0x10, S3C_UDC_OTG_GRSTCTL);
	while(readl(S3C_UDC_OTG_GRSTCTL) & 0x10);

	/* Flush all the Tx FIFO's */
	writel(0x10<<TxFIFO_NUM_BIT_5, S3C_UDC_OTG_GRSTCTL);
	writel((0x10<<TxFIFO_NUM_BIT_5)|(1<<TxFIFO_FLUSH_BIT), S3C_UDC_OTG_GRSTCTL);
	while(readl(S3C_UDC_OTG_GRSTCTL) & (1<<TxFIFO_FLUSH_BIT));

	s3c_ep0_pre_setup();
}

/**
 * s3c_udc_initialize
 * configure & initialize s3c-udc
 */
static void s3c_udc_initialize(struct s3c_udc *dev)
{
	/* 	2. Soft-reset OTG Core and then unreset again. */
	/* 	
	 * Typically software reset is used during software development
	 * and also when you dynamically change the PHY selection bits in
	 * the USB configuration registers 
	 */
//	writel(CORE_SOFT_RESET, S3C_UDC_OTG_GRSTCTL);		

//neet to confirm relationship among cable_check of battery, usb pmic, software connection
#if 0
	// 3. Put the OTG device core in the disconnected state.
	s3c_udc_soft_disconnect();
	udelay(20);
	
	// 4. Make the OTG device core exit from the disconnected state.
	s3c_udc_soft_connect();
#endif

	/* 	5. Configure OTG Core to initial settings of device mode. */
	/* 	[1: full speed(30Mhz) 0:high speed] */
	writel(1<<DCFG_EP_MISMATCH_CNT|DCFG_DEV_HIGH_SPEED_2_0<<DCFG_DEV_SPEED_BIT, S3C_UDC_OTG_DCFG);		

	udelay(100);
	
	/* 	 6. Unmask the core interrupts */
	writel(GINTMSK_INIT, S3C_UDC_OTG_GINTMSK);
}
//---------------------------------------------------------------------------------------

/**
 * s3c_udc_set_max_pktsize
 * set maxium packed size of EPs regarding USB speed configured
 */
 
void s3c_udc_set_max_pktsize(struct s3c_udc *dev, enum usb_device_speed speed)
{
	u32 ep0_in, ep0_out, i;
	u32 ep0_fifo_size = 0, bulk_fifo_size = 0;
	u32 int_fifo_size = 0, iso_fifo_size = 0;

/* USB spec. say (Ch.5)
	
	[Control Transfer]
		HIGH speed: 64 Bytes
		FULL speed: 8, 16, 32, 64 Bytes
	  	LOW  speed: 8 Bytes
	  	
	[Bulk Transfer]
		HIGH speed: 512 Bytes
		FULL speed: 8, 16, 32, 64 Bytes
	  	LOW  speed: not used
	  	
	[Interrupt Transfer]
		HIGH speed: up to 1024 Bytes
		FULL speed: up to 64 Bytes
	  	LOW  speed: up to 8 Bytes
	  	
	[Isochronous Transfer]
		HIGH speed: up to 1024 Bytes
		FULL speed: up to 1023 Bytes
	  	LOW  speed: not used
*/

	if (speed == USB_SPEED_HIGH) 
	{
		ep0_fifo_size = 64;
		bulk_fifo_size = 512;
		int_fifo_size = 1024;
		iso_fifo_size = 1024;
		dev->gadget.speed = USB_SPEED_HIGH;		
	} 
	else if (speed == USB_SPEED_FULL) 
	{
	/*
		8, 16, 32, 64 Bytes are available 
			for bulk_fifo_size, ep0_fifo_size regarding USB spec.
	*/
		ep0_fifo_size = 64;
		bulk_fifo_size = 64;
		int_fifo_size = 64;
		iso_fifo_size = 1023;
		dev->gadget.speed = USB_SPEED_FULL;
	}
	else //USB_SPEED_LOW
	{
		ep0_fifo_size = 8;
		int_fifo_size = 8;
		dev->gadget.speed = USB_SPEED_LOW;				
	}
	
	ep0_in = readl(S3C_UDC_OTG_DIEPCTL(EP0_CON));
	ep0_out = readl(S3C_UDC_OTG_DOEPCTL(EP0_CON));
	ep0_in &= ~DEPCTL0_MPS_MASK;
	ep0_out &= ~DEPCTL0_MPS_MASK;

	switch(ep0_fifo_size)
	{
		case 64:
			writel(ep0_in|DEPCTL0_MPS_64, S3C_UDC_OTG_DIEPCTL(EP0_CON));			
			writel(ep0_out|DEPCTL0_MPS_64, S3C_UDC_OTG_DOEPCTL(EP0_CON));
			break;
		case 32:
			writel(ep0_in|DEPCTL0_MPS_32, S3C_UDC_OTG_DIEPCTL(EP0_CON));			
			writel(ep0_out|DEPCTL0_MPS_32, S3C_UDC_OTG_DOEPCTL(EP0_CON));
			break;
		case 16:
			writel(ep0_in|DEPCTL0_MPS_16, S3C_UDC_OTG_DIEPCTL(EP0_CON));			
			writel(ep0_out|DEPCTL0_MPS_16, S3C_UDC_OTG_DOEPCTL(EP0_CON));
			break;
		case 8:
			writel(ep0_in|DEPCTL0_MPS_8, S3C_UDC_OTG_DIEPCTL(EP0_CON));			
			writel(ep0_out|DEPCTL0_MPS_8, S3C_UDC_OTG_DOEPCTL(EP0_CON));
			break;
		default:
			DEBUG_ERROR("[%s] Not proper value of ep0_fifo_size [%d] \n", __func__, ep0_fifo_size);
	}
	
	/*
		s3c_ep_activate() will change 
		the max packet size gadget driver assigned
	*/
	for(i=0; i<=S3C_MAX_ENDPOINTS;i++)
	{
		switch ((dev->ep[i].bmAttributes) & USB_ENDPOINT_XFERTYPE_MASK) 
		{
			case USB_ENDPOINT_XFER_BULK:	
				if (speed != USB_SPEED_LOW) 
					dev->ep[i].ep.maxpacket = bulk_fifo_size;
				break;				
			case USB_ENDPOINT_XFER_INT: 
				dev->ep[i].ep.maxpacket = int_fifo_size;
				break;								
			case USB_ENDPOINT_XFER_CONTROL:	
				dev->ep[i].ep.maxpacket = ep0_fifo_size;
				break;
			case USB_ENDPOINT_XFER_ISOC:
				if (speed != USB_SPEED_LOW) 
					dev->ep[i].ep.maxpacket = iso_fifo_size;
				break;					
		}//switch
	}//for
}
//---------------------------------------------------------------------------------------

static inline void s3c_ep_check_tx_queue(struct s3c_udc *dev, u8 ep_num)
{
	struct s3c_ep *ep = &dev->ep[ep_num];
	struct s3c_request *req;

	DEBUG("%s: Check queue, ep_num = %d\n", __func__, ep_num);

	if (!list_empty(&ep->queue)) 
	{
		req = list_entry(ep->queue.next, struct s3c_request, queue);
		DEBUG("%s: Next Tx request(0x%p) start...\n", __func__, req);

		if (ep_is_in(ep))
			s3c_udc_set_dma_tx(ep, req);
		else
			s3c_udc_set_dma_rx(ep, req);		
	} 
}

/**
 * handle_enum_done_intr
 * handler of enumerate done irq
 */
static void handle_enum_done_intr(struct s3c_udc *dev)
{
	u32	usb_status = readl(S3C_UDC_OTG_DSTS);
	/* 	EnumSpd 2:1 bits */
	usb_status &= 0x6;
	writel(INT_ENUMDONE, S3C_UDC_OTG_GINTSTS);

/*
 * Low speed is not supported for devices using a UTMI+ PHY
 * 6410 supports UTMI
 */	
	if (usb_status == USB_HIGH_30_60MHZ) 
	{
		printk("[%s] Enumerated as HIGH Speed \n", dev->driver->driver.name);
		DEBUG_SETUP("	 %s: High Speed Detection,  DSTS: 0x%x\n", __func__, usb_status);
		s3c_udc_set_max_pktsize(dev, USB_SPEED_HIGH);
	}
	else if (usb_status & USB_LOW_6MHZ) 
	{
		printk("[%s] Enumerated as LOW Speed \n", dev->driver->driver.name);
		DEBUG_SETUP("	 %s: Low Speed Detection,  DSTS: 0x%x\n", __func__, usb_status);
		s3c_udc_set_max_pktsize(dev, USB_SPEED_LOW);	
	}
	else 
	{
		printk("[%s] Enumerated as FULL Speed \n", dev->driver->driver.name);
		DEBUG_SETUP("	 %s: Full Speed Detection,  DSTS: 0x%x\n", __func__, usb_status);
		s3c_udc_set_max_pktsize(dev, USB_SPEED_FULL);	
	}
	s3c_ep0_pre_setup();
	
}
//---------------------------------------------------------------------------------------


/**
 * handle_reset_intr
 * handler of reset irq
 */
static void handle_reset_intr(struct s3c_udc *dev)
{	
	u32	usb_status = readl(S3C_UDC_OTG_GOTGCTL);
	writel(INT_RESET, S3C_UDC_OTG_GINTSTS);
	DEBUG_PM("[%s] : Reset interrupt - (GOTGCTL):0x%x\n", __func__, usb_status);

	/* confirm A & B Session Valid  */
	if((usb_status & 0xc0000) == (0x3 << 18)) 
	{
		DEBUG_PM("	   ===> OTG core got reset & execute s3c_udc_reset()\n");
		dev->udc_state = USB_STATE_DEFAULT;
		s3c_udc_reset(dev);//s3c_udc_initialize(dev);
		dev->ep0state = WAIT_FOR_SETUP;
	} 
	else 
	{
		DEBUG_PM("      RESET handling skipped : Not valid A or B session [GOTGCTL:0x%x] \n", usb_status);
	}
}
//---------------------------------------------------------------------------------------

/**
 * handle_resume_intr
 * handler of resume irq
 */
static void handle_resume_intr(struct s3c_udc *dev)
{
	writel(INT_RESUME, S3C_UDC_OTG_GINTSTS);
	//	s3c_udc_resume_clock_gating();
	
	DEBUG_PM("[%s]: USB Bus Resume\n", __func__);
	
	dev->udc_state = dev->udc_resume_state;
	dev->gadget.speed = dev->udc_resume_speed;

	if (dev->gadget.speed != USB_SPEED_UNKNOWN
		&& dev->driver && dev->driver->resume) 
	{
		S3C_UDC_UNLOCK(&dev->lock);
		dev->driver->resume(&dev->gadget);
		S3C_UDC_LOCK(&dev->lock);
	}
	
}
//---------------------------------------------------------------------------------------

/**
 * handle_suspend_intr
 * handler of suspend irq, USB suspend (host sleep)
 * Being disconnected make s3c-udc fall into suspend
 */
static void handle_suspend_intr(struct s3c_udc *dev)
{	
	u32 usb_status;
	
	writel(INT_SUSPEND, S3C_UDC_OTG_GINTSTS);
	
	//confirm device is attached or not
	if(dev->udc_state == USB_STATE_NOTATTACHED ||
		dev->udc_state == USB_STATE_POWERED ||
		dev->udc_state == USB_STATE_SUSPENDED )
	{
		//DEBUG_PM("[%s]: not proper state to go into the suspend mode\n", __func__);
		return;
	}
	
	usb_status = readl(S3C_UDC_OTG_DSTS);
	
	if ( !(usb_status & (1<<SUSPEND_STS)) )
	{
		//DEBUG_PM("[%s]: not suspend !~\n", __func__);
		return;
	}	

	if (dev->gadget.speed != USB_SPEED_UNKNOWN)
	{
		DEBUG_PM("[%s]: USB Bus Suspend (DSTS):0x%x\n", __func__, usb_status);
			
		dev->udc_resume_state = dev->udc_state;
		dev->udc_resume_speed = dev->gadget.speed;
		
		dev->udc_state = USB_STATE_SUSPENDED;
		
		//s3c_udc_suspend_clock_gating();
		if (dev->driver && dev->driver->suspend) 
		{
			S3C_UDC_UNLOCK(&dev->lock);
			dev->driver->suspend(&dev->gadget);
			S3C_UDC_LOCK(&dev->lock);			
		}

#if USBCV_CH9_REMOTE_WAKE_UP_TEST
		printk("[%s]: USBCV_CH9_REMOTE_WAKE_UP_TEST just return\n", __func__);
		return;
#endif
		// force s3c-udc to be disconnected for android gadgets
		if (dev->config_gadget_driver == ANDROID_ADB ||
			dev->config_gadget_driver == ANDROID_ADB_UMS ||
			dev->config_gadget_driver == ANDROID_ADB_UMS_ACM ||
			dev->config_gadget_driver == ANDROID_ADB_UMS_ACM_ETH_RNDIS)
		{
			s3c_udc_stop_activity(dev, dev->driver);
		}	
		// Confirm Other gadgets need to be disconnected or not
		else
		{
			s3c_udc_stop_activity(dev, dev->driver);
		}
	}
}
//---------------------------------------------------------------------------------------

u8 last_ep_in = S3C_MAX_ENDPOINTS;
/**
 * handle_ep_in_intr
 * handling reception of IN token
 */
static int handle_ep_in_intr(struct s3c_udc *dev)
{
	u32 ep_int, ep_int_status, ep_ctrl;
	u32 nptxQ_SpcAvail, nptxFifo_SpcAvail, gnptxsts;
	u8	ep_num = S3C_MAX_ENDPOINTS + 1, i;

	ep_int = readl(S3C_UDC_OTG_DAINT);
	DEBUG_IN_EP("\tDAINT : 0x%x \n", ep_int);
	
	for(i=(last_ep_in == S3C_MAX_ENDPOINTS)?0:last_ep_in+1; i<=S3C_MAX_ENDPOINTS; i++)
	{
		if (ep_int & (1<<i)) 
		{
			ep_num = i;
			last_ep_in = ep_num;
			break;
		}
	}		

//not found
	if (ep_num == S3C_MAX_ENDPOINTS + 1)
	{
		for(i=0; i<=last_ep_in; i++)
		{
			if (ep_int & (1<<i)) 
			{
				ep_num = i;
				last_ep_in = ep_num;
				break;
			}
		}	
	}	
	
	ep_int_status = readl(S3C_UDC_OTG_DIEPINT(ep_num));
	writel(ep_int_status, S3C_UDC_OTG_DIEPINT(ep_num)); 	// Interrupt Clear

	if (ep_num > S3C_MAX_ENDPOINTS) {
		DEBUG_ERROR("[%s]: No matching EP DAINT : 0x%x \n", __func__, ep_int);
		return -1;
	}

	gnptxsts = readl(S3C_UDC_OTG_GNPTXSTS);
	
	nptxQ_SpcAvail = (gnptxsts & (0xff<<16))>>16;
	nptxFifo_SpcAvail = gnptxsts & 0xffff;
	
	DEBUG_IN_EP("	 GNPTXSTS nptxQ_SpcAvail = %d, nptxFifo_SpcAvail = %d\n",nptxQ_SpcAvail ,nptxFifo_SpcAvail);
	
	if (nptxQ_SpcAvail == 0 || nptxFifo_SpcAvail == 0)
	{
		DEBUG_ERROR("[%s] : nptxQ_SpcAvail == 0 || nptxFifo_SpcAvail == 0 \n", __func__);
		return -1;
	}
	
#if SERIAL_TRANSFER_DELAY 
	/*
		The below delaying is dedicated to g_serial for handling 'ls' command in 
		directory which has many files
	*/
	if(ep_num != EP0_CON && (dev->config_gadget_driver == SERIAL || dev->config_gadget_driver == CDC2) )
	{			
	#if OTG_DBG_ENABLE
		#ifdef DEBUG_S3C_UDC_IN_EP
					DEBUG_IN_EP("[%s] : EP In for g_serial or g_cdc\n", __func__);
		#else
					printk("[%s] : EP In for g_serial or g_cdc\n", __func__);
		#endif			
	#else
		udelay(SERIAL_DELAY_uTIME);
	#endif //OTG_DBG_ENABLE
	}

	if( (ep_num == EP2_IN && dev->config_gadget_driver == ANDROID_ADB_UMS_ACM) || 
		(ep_num == EP11_IN && dev->config_gadget_driver == ANDROID_ADB_UMS_ACM_ETH_RNDIS) )	
	{			
	#if OTG_DBG_ENABLE
	#ifdef DEBUG_S3C_UDC_IN_EP
		DEBUG_IN_EP("[%s] : EP In for g_serial or g_cdc\n", __func__);
	#else
		printk("[%s] : EP In for g_serial or g_cdc\n", __func__);
	#endif			
	#else
		udelay(SERIAL_DELAY_uTIME);
	#endif //OTG_DBG_ENABLE
	}
#endif //SERIAL_TRANSFER_DELAY

	switch(ep_num)
	{
		case EP0_CON: 
		case EP2_IN: 	case EP3_IN: 
		case EP5_IN:	case EP6_IN:  
		case EP8_IN:	case EP9_IN:			
		case EP11_IN:  	case EP12_IN:			
		case EP14_IN:	case EP15_IN:			

			ep_ctrl = readl(S3C_UDC_OTG_DIEPCTL(ep_num));
			DEBUG_IN_EP("\tEP[%d]-IN : DIEPINT[%d] = 0x%x, DIEPCTL[%d] = 0x%x \n", 
				ep_num, ep_num, ep_int_status, ep_num,  ep_ctrl);
			
			if (ep_int_status & TRANSFER_DONE) 
			{
				DEBUG_IN_EP("\tEP[%d]-IN transaction completed - (TX DMA done)\n", ep_num);
			
				handle_tx_complete(dev, ep_num);

				if (ep_num == EP0_CON) 
				{
					if(dev->ep0state == WAIT_FOR_SETUP) 
					{
						s3c_ep0_pre_setup();
					}
#if 0
					/* continue transfer after set_clear_halt for DMA mode */
					if (clear_feature_flag == 1)
					{
						s3c_ep_check_tx_queue(dev, clear_feature_num);
						clear_feature_flag = 0;
					}
#endif				
				}
			}
			break;
		default:
			DEBUG_ERROR("[%s]: Error Unused EP[%d]\n", __func__, ep_num);
			return ep_num;
	} //switch
	return 0;
}
//---------------------------------------------------------------------------------------

u8 last_ep_out = S3C_MAX_ENDPOINTS;
/**
 * handle_ep_out_intr
 * handling reception of OUT token
 */
static void handle_ep_out_intr(struct s3c_udc * dev)
{
	u32 ep_int, ep_int_status, ep_ctrl;
	u8	ep_num = S3C_MAX_ENDPOINTS + 1, i;

	ep_int = readl(S3C_UDC_OTG_DAINT);
	DEBUG_OUT_EP("\tDAINT : 0x%x \n", ep_int);
			
	for(i=(last_ep_out == S3C_MAX_ENDPOINTS)?0:last_ep_out+1; i<=S3C_MAX_ENDPOINTS; i++)
	{
		if (ep_int & ((1<<i)<<16)) 
		{
			ep_num = i;
			last_ep_out = ep_num;
			break;
		}
	}			

//not found
	if (ep_num == S3C_MAX_ENDPOINTS + 1)
	{
		for(i=0; i<=last_ep_out; i++)
		{
			if (ep_int & ((1<<i)<<16)) 
			{
				ep_num = i;
				last_ep_out = ep_num;
				break;
			}
		}	
	}	

	ep_int_status = readl(S3C_UDC_OTG_DOEPINT(ep_num));

	if (ep_num > S3C_MAX_ENDPOINTS) 
	{
		DEBUG_ERROR("[%s]: No matching EP DAINT : 0x%x \n", __func__, ep_int);
		writel(ep_int_status, S3C_UDC_OTG_DOEPINT(ep_num)); 	// ep1 Interrupt Clear
		return;
	}
	
	switch(ep_num)
	{
		case EP0_CON: 
			ep_ctrl = readl(S3C_UDC_OTG_DOEPCTL(EP0_CON));
			DEBUG_EP0("\tEP0-OUT : DOEPINT0 = 0x%x, DOEPCTL0 = 0x%x\n", ep_int_status, ep_ctrl);
			
			if (ep_int_status & CTRL_OUT_EP_SETUP_PHASE_DONE) 
			{
				DEBUG_EP0("\tSETUP packet(transaction) arrived\n");
				writel(CTRL_OUT_EP_SETUP_PHASE_DONE, S3C_UDC_OTG_DOEPINT(ep_num)); // Interrupt Clear
				s3c_ep0_handle(dev);
			}
			else if (ep_int_status & TRANSFER_DONE) 
			{
				writel(TRANSFER_DONE, S3C_UDC_OTG_DOEPINT(ep_num));	// Interrupt Clear
				handle_rx_complete(dev, EP0_CON);
				s3c_ep0_pre_setup();
			}
			else
			{
				writel(ep_int_status, S3C_UDC_OTG_DOEPINT(ep_num));	// Interrupt Clear
				s3c_ep0_pre_setup();
			}
			break;
		case EP1_OUT: case EP4_OUT:	case EP7_OUT: case EP10_OUT: case EP13_OUT:
			ep_ctrl = readl(S3C_UDC_OTG_DOEPCTL(ep_num));
			
			DEBUG_OUT_EP("\tEP[%d]-OUT : DOEPINT[%d] = 0x%x, DOEPCTL[%d] = 0x%x\n", 
				ep_num, ep_num, ep_int_status, ep_num, ep_ctrl);
			
			if (ep_int_status & TRANSFER_DONE) 
			{
				DEBUG_OUT_EP("\tBULK OUT packet(transaction) arrived - (RX DMA done)\n");
				handle_rx_complete(dev, ep_num);
			}
			writel(ep_int_status, S3C_UDC_OTG_DOEPINT(ep_num));		// Interrupt Clear
			break;
		default:
			writel(ep_int_status, S3C_UDC_OTG_DOEPINT(ep_num));		// Interrupt Clear
			DEBUG_ERROR("[%s]: Error Unused EP[%d]\n", __func__, ep_num);
	}
}
//---------------------------------------------------------------------------------------

/**
 * s3c_udc_irq
 * irq handler of s3c-udc
 */
static irqreturn_t s3c_udc_irq(int irq, void *_dev)
{
	struct s3c_udc *dev = _dev;
	u32 intr_status, gintmsk;
	unsigned long flags;

	S3C_UDC_LOCK_IRQSAVE(&dev->lock, flags);

	intr_status = readl(S3C_UDC_OTG_GINTSTS);
	gintmsk = readl(S3C_UDC_OTG_GINTMSK);

	DEBUG_ISR("\n**** %s : GINTSTS=0x%x(on state %s), GINTMSK : 0x%x, DAINT : 0x%x, DAINTMSK : 0x%x\n",
			__func__, intr_status, state_names[dev->ep0state], gintmsk, 
			readl(S3C_UDC_OTG_DAINT), readl(S3C_UDC_OTG_DAINTMSK));

	if(intr_status & INT_OUT_EP)
	{
		DEBUG_OUT_EP("[%s] : EP Out interrupt \n", __func__);
		handle_ep_out_intr(dev);
		goto	OK_OUT;
	}

	if (intr_status & INT_IN_EP) 
	{
		int ret;
		u32 ep_int_status;
		DEBUG_IN_EP("[%s] : EP In interrupt \n", __func__);
		ret = handle_ep_in_intr(dev);
		
		if(ret == -1)
			goto FAIL_OUT;
		else if (ret > 0)
			{			
				printk("[%s] : SW walk around ~~~~~\n", __func__);
				printk("[%s] : EP In interrupt  ~~~~~\n", __func__);
				printk("[%s] : EP (%d) ~~~~~\n", __func__, ret);
				
				ep_int_status = readl(S3C_UDC_OTG_DIEPINT(ret));
				writel(ep_int_status, S3C_UDC_OTG_DIEPINT(ret)); 	
				ep_int_status = readl(S3C_UDC_OTG_DOEPINT(ret));
				writel(ep_int_status, S3C_UDC_OTG_DOEPINT(ret)); 	
				goto	OK_OUT;
			}
		else
			goto	OK_OUT;
	}
	
	if (intr_status & INT_RESET) {
		handle_reset_intr(dev);			
		goto	OK_OUT;
	}

	if (intr_status & INT_ENUMDONE) 
	{
		DEBUG_SETUP("[%s] : Enumeration Done interrupt\n",	__func__);
		handle_enum_done_intr(dev);
		goto	OK_OUT;
	}

/*
	Currently(Apr 13, 2009)
	No activities needed during early suspend.
	
	if (intr_status & INT_EARLY_SUSPEND) 
	{
		DEBUG_PM("[%s] Early suspend interrupt\n", __func__);
		writel(INT_EARLY_SUSPEND, S3C_UDC_OTG_GINTSTS);
		goto	OK_OUT;
	}
*/
	if (intr_status & INT_SUSPEND) 
	{
		handle_suspend_intr(dev);
		goto	OK_OUT;
	}
	
	if (intr_status & INT_RESUME) 
	{
		handle_resume_intr(dev);
		goto	OK_OUT;
	}
	
	if (intr_status) 
		DEBUG_ERROR("no handler for S3C_UDC_OTG_GINTSTS [%d]\n", intr_status);
	else
		DEBUG_ERROR("no S3C_UDC_OTG_GINTSTS( == 0)\n");

	goto	FAIL_OUT;
	
OK_OUT:
	S3C_UDC_UNLOCK_IRQRESTORE(&dev->lock, flags);
	return IRQ_HANDLED;

FAIL_OUT:
	DEBUG_ERROR("[%s] : IRQ_NONE \n", __func__);
	S3C_UDC_UNLOCK_IRQRESTORE(&dev->lock, flags);
	return IRQ_NONE;
}
//---------------------------------------------------------------------------------------

void s3c_ep_activate(struct s3c_ep *ep)
{
	u8 ep_num;
	u32 ep_ctrl = 0, daintmsk = 0;
	
	ep_num = ep_index(ep);

	/* Read DEPCTLn register */
	if (ep_is_in(ep)) {
		ep_ctrl = readl(S3C_UDC_OTG_DIEPCTL(ep_num));
		daintmsk = 1 << ep_num;
	} else {
		ep_ctrl = readl(S3C_UDC_OTG_DOEPCTL(ep_num));
		daintmsk = (1 << ep_num) << DAINT_OUT_BIT;
	}

	DEBUG_SETUP("%s: EPCTRL%d = 0x%x, ep_is_in = %d\n",
		__func__, ep_num, ep_ctrl, ep_is_in(ep));
		
	/* If the EP is already active don't change the EP Control
	 * register. */
	if (!(ep_ctrl & DEPCTL_USBACTEP)) {
		ep_ctrl = (ep_ctrl & ~DEPCTL_TYPE_MASK)| (ep->bmAttributes << DEPCTL_TYPE_BIT);
		ep_ctrl = (ep_ctrl & ~DEPCTL_MPS_MASK) | (ep->ep.maxpacket << DEPCTL_MPS_BIT);
		ep_ctrl |= (DEPCTL_SETD0PID | DEPCTL_USBACTEP);

		if (ep_is_in(ep)) {
			writel(ep_ctrl, S3C_UDC_OTG_DIEPCTL(ep_num));
			DEBUG_SETUP("%s: USB Ative EP%d, DIEPCTRL%d = 0x%x\n",
				__func__, ep_num, ep_num, readl(S3C_UDC_OTG_DIEPCTL(ep_num)));
		} else {
			writel(ep_ctrl, S3C_UDC_OTG_DOEPCTL(ep_num));
			DEBUG_SETUP("%s: USB Ative EP%d, DOEPCTRL%d = 0x%x\n",
				__func__, ep_num, ep_num, readl(S3C_UDC_OTG_DOEPCTL(ep_num)));
		}
	}
	else		
		DEBUG_SETUP("%s: already activated EP%d, DIEPCTRL = 0x%x\n",
			__func__, ep_num, readl(S3C_UDC_OTG_DIEPCTL(ep_num)));

	/* Unmask EP Interrtupt */
	writel(readl(S3C_UDC_OTG_DAINTMSK)|daintmsk, S3C_UDC_OTG_DAINTMSK);
	DEBUG_SETUP("%s: DAINTMSK = 0x%x\n", __func__, readl(S3C_UDC_OTG_DAINTMSK));
}

/**
 * enable an ep
 */
static int s3c_ep_enable(struct usb_ep *_ep,
			     const struct usb_endpoint_descriptor *desc)
{
	struct s3c_ep *ep;
	struct s3c_udc *dev;
	unsigned long flags;
	
	u8  ep_num;
	
	ep = container_of(_ep, struct s3c_ep, ep);

	DEBUG_SETUP("[%s] EP-%d\n", __func__, ep_index(ep));

	if (!_ep || !desc || ep->desc || _ep->name == ep0name
	    || desc->bDescriptorType != USB_DT_ENDPOINT
	    || ep->bEndpointAddress != desc->bEndpointAddress)
	{
		DEBUG_ERROR("[%s] bad ep or descriptor\n", __func__);

		if (!_ep)
		{
			DEBUG_ERROR("[%s] (!_ep) \n", __func__);
			return -EINVAL;
		}

		if (!desc)
		{
			DEBUG_ERROR("[%s] (!desc ) \n", __func__);
			return -EINVAL;
		}

		if (_ep->name == ep0name)
		{
			DEBUG_ERROR("[%s] (_ep->name == ep0name) \n", __func__);
		}

		if (desc->bDescriptorType != USB_DT_ENDPOINT)
		{
			DEBUG_ERROR("[%s] desc->bDescriptorType != USB_DT_ENDPOINT\n", __func__);
		}

		if (ep->bEndpointAddress != desc->bEndpointAddress)
		{
			DEBUG_ERROR("[%s] ep->bEndpointAddress != desc->bEndpointAddress \n", __func__);
		}
		
		return -EINVAL;
	}

	if (ep_maxpacket(ep) < le16_to_cpu(desc->wMaxPacketSize)) 
	{
		DEBUG_ERROR("[%s] ep_maxpacket(ep) < desc->wMaxPacketSize\n", __func__);
		return -ERANGE;
	}

	/* xfer types must match, except that interrupt ~= bulk */
	if (ep->bmAttributes != desc->bmAttributes
	    && ep->bmAttributes != USB_ENDPOINT_XFER_BULK
	    && desc->bmAttributes != USB_ENDPOINT_XFER_INT) 
	{
		DEBUG_ERROR("[%s] %s type mismatch\n", __func__, _ep->name);
		return -EINVAL;
	}

	/* hardware _could_ do smaller, but driver doesn't */
	if ((desc->bmAttributes == USB_ENDPOINT_XFER_BULK
	     && le16_to_cpu(desc->wMaxPacketSize) != ep_maxpacket(ep))
	    || !desc->wMaxPacketSize) 
	{
		DEBUG_ERROR("[%s] bad %s maxpacket\n", __func__, _ep->name);
		return -ERANGE;
	}

	dev = ep->dev;
	if (!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN) 
	{
		DEBUG_ERROR("[%s] bogus device state\n", __func__);
		return -ESHUTDOWN;
	}

	S3C_UDC_LOCK_IRQSAVE(&ep->dev->lock, flags);

	ep->stopped = 0;
	ep->desc = desc;
	ep->pio_irqs = 0;
	ep->ep.maxpacket = le16_to_cpu(desc->wMaxPacketSize);

	/* Reset halt state */
	s3c_ep_set_halt(_ep, 0);

	ep_num = ep_index(ep);
	if(ep_num <= S3C_MAX_ENDPOINTS)
		s3c_ep_activate(ep);
	else
		DEBUG_ERROR("[%s] not supported ep %d\n",__func__, ep_num); 		
	S3C_UDC_UNLOCK_IRQRESTORE(&ep->dev->lock, flags);

	DEBUG_SETUP("[%s] enabled %s, stopped = %d, maxpacket = %d\n",
		__func__, _ep->name, ep->stopped, ep->ep.maxpacket);
	return 0;
}
//---------------------------------------------------------------------------------------

/**
 * disable a ep
 */
static int s3c_ep_disable(struct usb_ep *_ep)
{
	struct s3c_ep *ep = NULL;
	unsigned long flags;
	u8 ep_num;
	
	u32 ep_ctrl, daintmsk;

	ep = container_of(_ep, struct s3c_ep, ep);
	
	DEBUG_SETUP("[%s] EP-%d\n", __func__, ep_index(ep));

	if (!_ep || !ep->desc) 
	{
		DEBUG_SETUP("[%s] %s not enabled\n", __func__,
		      _ep ? ep->ep.name : NULL);
		return -EINVAL;
	}
	
	ep_num = ep_index(ep);

	S3C_UDC_LOCK_IRQSAVE(&ep->dev->lock, flags);

	/* Nuke all pending requests */
	s3c_ep_nuke(ep, -ESHUTDOWN);

	ep->desc = 0;
	ep->stopped = 1;
	
	daintmsk = readl(S3C_UDC_OTG_DAINTMSK);

	if(ep_num <= S3C_MAX_ENDPOINTS)
	{
		if(ep_is_in(ep))
		{
			/* EP: IN */
			ep_ctrl = readl(S3C_UDC_OTG_DIEPCTL(ep_num));
			writel(ep_ctrl&~(DEPCTL_USBACTEP), S3C_UDC_OTG_DIEPCTL(ep_num)); 
			writel(daintmsk&(~(1<<ep_num)), S3C_UDC_OTG_DAINTMSK);
		}
		else
		{
			/* EP: OUT */
			ep_ctrl = readl(S3C_UDC_OTG_DOEPCTL(ep_num));
			writel(ep_ctrl&~(DEPCTL_USBACTEP), S3C_UDC_OTG_DOEPCTL(ep_num)); 
			writel(daintmsk&(~(1<<(ep_num+16))), S3C_UDC_OTG_DAINTMSK);
		}
	}
	else
	{
		DEBUG_ERROR("[%s] not supported ep %d\n",__func__, ep_num);		
	}
	S3C_UDC_UNLOCK_IRQRESTORE(&ep->dev->lock, flags);
	
	return 0;
}
//---------------------------------------------------------------------------------------

/**
 * s3c_ep_alloc_request
 * allocate a request for s3c-udc
 */
static struct usb_request *s3c_ep_alloc_request(struct usb_ep *ep,
						 gfp_t gfp_flags)
{
	struct s3c_request *req;

	DEBUG("[%s] %s %p\n", __func__, ep->name, ep);

	req = kmalloc(sizeof *req, gfp_flags);
	if (!req)
		return 0;
	
	memset(req, 0, sizeof *req);
	INIT_LIST_HEAD(&req->queue);

	return &req->req;
}
//---------------------------------------------------------------------------------------

/**
 * s3c_ep_free_request
 * free a request for s3c-udc
 */
static void s3c_ep_free_request(struct usb_ep *ep, struct usb_request *_req)
{
	struct s3c_request *req;

	DEBUG("[%s] %p\n", __func__, ep);

	req = container_of(_req, struct s3c_request, req);
	kfree(req);
}
//---------------------------------------------------------------------------------------

/*
 * Queue one request
 * Kickstart transfer if needed
 */
static int s3c_ep_queue(struct usb_ep *_ep, struct usb_request *_req,
			 gfp_t gfp_flags)
{
	struct s3c_request *req;
	struct s3c_ep *ep;
	struct s3c_udc *dev;
	unsigned long flags;
	u32 ep_num;

	if(!_req)
		return -EINVAL;
		
	req = container_of(_req, struct s3c_request, req);
		
	if (unlikely(!_req || !_req->complete || !_req->buf || !list_empty(&req->queue)))
	{
		DEBUG_ERROR("[%s] bad params\n", __func__);
		
		DEBUG_ERROR("_req=[%x]\n",_req);
		DEBUG_ERROR("_req->complete=[%x]\n",_req, _req->complete);
		DEBUG_ERROR("_req->buf=[%x]\n", _req->buf);
		
		if (!list_empty(&req->queue))
			DEBUG_ERROR("[%s] (!list_empty(&req->queue))\n", __func__);
		else if (!_req->buf )
			DEBUG_ERROR("[%s] (!_req->buf )\n", __func__);
		else if (!_req->complete)
			DEBUG_ERROR("[%s] (!_req->complete)\n", __func__);
		else // (!_req )
			DEBUG_ERROR("[%s] (!_req )\n", __func__);
		
		return -EINVAL;
	}

	ep = container_of(_ep, struct s3c_ep, ep);	
	
	if (unlikely(!_ep || (!ep->desc && ep->ep.name != ep0name))) 
	{
		DEBUG_ERROR("[%s] : bad ep\n", __func__);
		return -EINVAL;
	}

	ep_num = (u32)ep_index(ep);
	dev = ep->dev;
	
	if (unlikely(!dev->driver)) {
		DEBUG_ERROR("[%s] EP[%d]: Queueing dev->driver is NULL \n", __func__, ep_num);
		return -ESHUTDOWN;
	}
//	if (unlikely(dev->gadget.speed == USB_SPEED_UNKNOWN && 
//			dev->udc_state < USB_STATE_ADDRESS)) {
	if (unlikely(dev->gadget.speed == USB_SPEED_UNKNOWN)) {
		DEBUG_ERROR("[%s] EP[%d]: Queueing USB_SPEED_UNKNOWN state\n", __func__, ep_num);
		return -ESHUTDOWN;
	}
	
	DEBUG("\n%s: %s queue req %p, len %d buf %p\n",__func__, _ep->name, _req, _req->length, _req->buf);

	S3C_UDC_LOCK_IRQSAVE(&dev->lock, flags);

	_req->status = -EINPROGRESS;
	_req->actual = 0;

	/* kickstart this i/o queue? */
	DEBUG("[%s] Add to ep=%d, Q empty=%d, stopped=%d\n",__func__, ep_num, list_empty(&ep->queue), ep->stopped);

	/* 		
		The concept of req.zero is transfering additional ZLP as last packet.\
		The following is a S/W walk-around that just sends ZLP not additional
		for urgent sending 
	*/
	if(req->req.zero == true && req->req.length == 0) 
	{				
		DEBUG("[%s] gadget driver request sending zlp\n", __func__);		
		s3c_ep_send_zlp(ep_num);
		S3C_UDC_UNLOCK_IRQRESTORE(&dev->lock, flags);
		return 0;
	}		

#if S3C_UDC_ZLP
	req->zlp = false;
	if(req->req.zero)
	{
		if((req->req.length % ep->ep.maxpacket == 0) &&
			(req->req.length != 0) )
		{				
			req->zlp = true;
			DEBUG("[%s] S3C_UDC_ZLP req->zlp = true\n", __func__);
		}		
	}
#endif	

	if (list_empty(&ep->queue) && !ep->stopped) 
	{
		u32 csr;
		
		/* EP0 */
		if (ep_num == 0) 
		{
			list_add_tail(&req->queue, &ep->queue);
			DEBUG_EP0("[%s] ep_is_in = %d\n", __func__, ep_is_in(ep));
			
			if (ep_is_in(ep)) 
			{
				dev->ep0state = DATA_STATE_XMIT;
				s3c_ep0_write(dev);
			} 
			else 
			{
				dev->ep0state = DATA_STATE_RECV;
				s3c_ep0_read(dev);
			}
			req = 0;

		} 
		/* EP-IN */
		else if(ep_is_in(ep))// 
		{
			csr = readl(S3C_UDC_OTG_GINTSTS);
			DEBUG_IN_EP("[%s] : ep_is_in, S3C_UDC_OTG_GINTSTS=0x%x\n",
				__func__, csr);
			s3c_udc_set_dma_tx(ep, req);
		} 
		/* EP-OUT */
		else 
		{
			csr = readl(S3C_UDC_OTG_GINTSTS);
			DEBUG_OUT_EP("[%s] ep_is_out, S3C_UDC_OTG_GINTSTS=0x%x\n",
				__func__, csr);

			s3c_udc_set_dma_rx(ep, req);
		}
	}

	/* pio or dma irq handler advances the queue. */
	if (likely(req != 0))
	{
		list_add_tail(&req->queue, &ep->queue);
	}

	S3C_UDC_UNLOCK_IRQRESTORE(&dev->lock, flags);

	return 0;
}
//---------------------------------------------------------------------------------------

/**
 * s3c_ep_dequeue
 * dequeue JUST ONE request
 */
static int s3c_ep_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct s3c_ep *ep;
	struct s3c_request *req;
	unsigned long flags;

	DEBUG("[%s] %p\n", __func__, _ep);

	ep = container_of(_ep, struct s3c_ep, ep);
	if (!_ep || ep->ep.name == ep0name)
	{
		DEBUG("[%s] !_ep || !ep\n", __func__);
		return -EINVAL;
	}
	
	if (unlikely(ep->dev->gadget.speed == USB_SPEED_UNKNOWN)) {
		DEBUG_ERROR("[%s] EP[%d]: UDC is in USB_SPEED_UNKNOWN state\n", __func__, ep_index(ep));
		return -ESHUTDOWN;
	}

	if (unlikely(list_empty(&ep->queue))) {		
		DEBUG_ERROR("[%s] no req on ep[%d]\n", __func__, ep_index(ep));
		return -ESHUTDOWN;
	}

	S3C_UDC_LOCK_IRQSAVE(&ep->dev->lock, flags);

	/* make sure it's actually queued on this endpoint */
	list_for_each_entry(req, &ep->queue, queue) 
	{
		if (&req->req == _req)
			break;
	}
	
	if (&req->req != _req) 
	{
		S3C_UDC_UNLOCK_IRQRESTORE(&ep->dev->lock, flags);
		return -EINVAL;
	}

	s3c_req_done(ep, req, -ECONNRESET);

	S3C_UDC_UNLOCK_IRQRESTORE(&ep->dev->lock, flags);
	return 0;
}
//---------------------------------------------------------------------------------------

void s3c_ep_set_stall(struct s3c_ep *ep)
{
	u8		ep_num;
	u32		ep_ctrl = 0;

	ep_num = ep_index(ep);
	DEBUG_SETUP("%s: ep_num = %d\n", __func__, ep_num);
		
	if (ep_is_in(ep)) {
		ep_ctrl = readl(S3C_UDC_OTG_DIEPCTL(ep_num));
	
		/* set the disable and stall bits */
//		if (ep_ctrl & DEPCTL_EPENA) {
//			ep_ctrl |= DEPCTL_EPDIS;
//		}
		ep_ctrl |= DEPCTL_STALL;

		writel(ep_ctrl, S3C_UDC_OTG_DIEPCTL(ep_num));
		DEBUG_SETUP("%s: set stall, DIEPCTL%d = 0x%x\n",
			__func__, ep_num, readl(S3C_UDC_OTG_DIEPCTL(ep_num)));

	} else {
		ep_ctrl = readl(S3C_UDC_OTG_DOEPCTL(ep_num));

		/* set the stall bit */
		ep_ctrl |= DEPCTL_STALL;

		writel(ep_ctrl, S3C_UDC_OTG_DOEPCTL(ep_num));
		DEBUG_SETUP("%s: set stall, DOEPCTL%d = 0x%x\n",
			__func__, ep_num, readl(S3C_UDC_OTG_DOEPCTL(ep_num)));
	}
		
	return;
}

void s3c_ep_clear_stall(struct s3c_ep *ep)
{
	u8		ep_num;
	u32		ep_ctrl = 0;

	ep_num = ep_index(ep);	
	DEBUG_SETUP("%s: ep_num = %d, \n", __func__, ep_num);

	if (ep_num == EP0_CON)
	{	
		DEBUG_SETUP("%s: ep_num = %d Core will clear\n", __func__, ep_num);
		return;
	}
#ifdef CONFIG_USB_FILE_STORAGE
	if(ep_num == EP5_IN && !MSC_INVALID_CBW_IGNORE_CLEAR_HALT()) {
		DEBUG("[%s] skipped by ep[%d] MSC_INVALID_CBW_IGNORE_CLEAR_HALT \n", __func__, ep_num);
		return;
	}
#endif

	if (ep_is_in(ep)) {
		ep_ctrl = readl(S3C_UDC_OTG_DIEPCTL(ep_num));

		/* clear stall bit */
		ep_ctrl &= ~DEPCTL_STALL;

		/* 
		 * USB Spec 9.4.5: For endpoints using data toggle, regardless
		 * of whether an endpoint has the Halt feature set, a
		 * ClearFeature(ENDPOINT_HALT) request always results in the
		 * data toggle being reinitialized to DATA0.
		 */
		if (ep->bmAttributes == USB_ENDPOINT_XFER_INT
		    || ep->bmAttributes == USB_ENDPOINT_XFER_BULK) {
			ep_ctrl |= DEPCTL_SETD0PID; /* DATA0 */
		}

		writel(ep_ctrl, S3C_UDC_OTG_DIEPCTL(ep_num));
		DEBUG_SETUP("%s: cleared stall, DIEPCTL%d = 0x%x\n",
			__func__, ep_num, readl(S3C_UDC_OTG_DIEPCTL(ep_num)));

	} else {
		ep_ctrl = readl(S3C_UDC_OTG_DOEPCTL(ep_num));

		/* clear stall bit */
		ep_ctrl &= ~DEPCTL_STALL;

		if (ep->bmAttributes == USB_ENDPOINT_XFER_INT
		    || ep->bmAttributes == USB_ENDPOINT_XFER_BULK) {
			ep_ctrl |= DEPCTL_SETD0PID; /* DATA0 */
		}

		writel(ep_ctrl, S3C_UDC_OTG_DOEPCTL(ep_num));
		DEBUG_SETUP("%s: cleared stall, DOEPCTL%d = 0x%x\n",
			__func__, ep_num, readl(S3C_UDC_OTG_DOEPCTL(ep_num)));
	}

	return;
}

static inline void s3c_ep0_set_stall(struct s3c_ep *ep) 
{
	struct s3c_udc *dev;
	u32		ep_ctrl = 0;

	dev = ep->dev;
	ep_ctrl = readl(S3C_UDC_OTG_DIEPCTL(EP0_CON));

	/* set the disable and stall bits ONLY IN direction */
	if (ep_ctrl & DEPCTL_EPENA) {
		ep_ctrl |= DEPCTL_EPDIS;
	}
	ep_ctrl |= DEPCTL_STALL;

	writel(ep_ctrl, S3C_UDC_OTG_DIEPCTL(EP0_CON));

	DEBUG_SETUP("%s: set ep%d stall, DIEPCTL0 = 0x%x\n",
		__func__, ep_index(ep), readl(S3C_UDC_OTG_DIEPCTL(EP0_CON)));
	/* 
	 * The application can only set this bit, and the core clears it,
	 * when a SETUP token is received for this endpoint
	 */
	dev->ep0state = WAIT_FOR_SETUP;
	s3c_ep0_pre_setup();
}

/** Halt specific EP
 *  Return 0 if success
 */	
static int s3c_ep_set_halt(struct usb_ep *_ep, int value)
{
	struct s3c_ep	*ep;
	struct s3c_udc	*dev;
	u8		ep_num;

	ep = container_of(_ep, struct s3c_ep, ep);

	if (unlikely (!_ep || (!ep->desc && ep->ep.name != ep0name) ||
			ep->desc->bmAttributes == USB_ENDPOINT_XFER_ISOC)) {
		DEBUG_ERROR("%s: %s bad ep or descriptor\n", __func__, ep->ep.name);
		return -EINVAL;
	}

	/* Attempt to halt IN ep will fail if any transfer requests
	 * are still queue */
	if (value && ep_is_in(ep) && !list_empty(&ep->queue)) {
		DEBUG_ERROR("%s: %s queue not empty, req = %p\n",
			__func__, ep->ep.name,
			list_entry(ep->queue.next, struct s3c_request, queue));

		return -EAGAIN;
	}

	dev = ep->dev;
	ep_num = ep_index(ep);
	DEBUG_SETUP("%s: ep_num = %d, value = %d\n", __func__, ep_num, value);

	/* clear */
	if (value == 0)
	{
		ep->stopped = 0;
		// EP0 stall will be cleared by OTG Core
		if (ep_num != 0) 
			s3c_ep_clear_stall(ep);
	} 
	/* set */
	else 
	{
		ep->stopped = 1;
		if (ep_num == 0) 
		{
			s3c_ep0_set_stall(ep);
		}
		else
		{
			s3c_ep_set_stall(ep);
		}
	}

	return 0;
}
//---------------------------------------------------------------------------------------

/**
 *  write a request into ep0's fifo
 *  return:  0 = still running, 1 = completed, negative = errno
 */
static int s3c_ep0_write_fifo(struct s3c_ep *ep, struct s3c_request *req)
{
	u32 max;
	unsigned count;
	int is_last;

	max = ep_maxpacket(ep);

	DEBUG_EP0("[%s] max = %d\n", __func__, max);

	count = s3c_udc_set_dma_tx(ep, req);

	/* last packet is usually short (or a zlp) */
	if (likely(count != max))
		is_last = 1;
	else 
	{
		if (likely(req->req.length != req->req.actual) || req->req.zero)
			is_last = 0;
		else
			is_last = 1;
	}

	DEBUG_EP0("[%s] wrote %s %d bytes%s %d left %p\n", __func__,
		  ep->ep.name, count,
		  is_last ? "/L" : "", req->req.length - req->req.actual, req);

	/* requests complete when all IN data is in the FIFO */
	if (is_last) 
	{
		ep->dev->ep0state = WAIT_FOR_SETUP;
		return 1;
	}

	return 0;
}
//---------------------------------------------------------------------------------------

/**
 * s3c_udc_set_address - set the USB address for this device
 *
 * Called from control endpoint function
 * after it decodes a set address setup packet.
 */
static void s3c_udc_set_address(struct s3c_udc *dev, unsigned char address)
{
	u32 ctrl = readl(S3C_UDC_OTG_DCFG);

	ctrl &= ~(DCFG_DEV_ADDRESS_MASK << DCFG_DEV_ADDRESS_BIT);
	ctrl |= (address << DCFG_DEV_ADDRESS_BIT);
		
	writel(ctrl, S3C_UDC_OTG_DCFG);

	DEBUG_EP0("[%s] USB OTG 2.0 Device address=%d, DCFG=0x%x\n",
		__func__, address, readl(S3C_UDC_OTG_DCFG));
	
	dev->usb_address = address;
	dev->udc_state = USB_STATE_ADDRESS;
}
//---------------------------------------------------------------------------------------

//dma read g_status out of function
static	u16	g_status __attribute__((aligned(8)));

static int s3c_udc_get_status(struct s3c_udc *dev,
		struct usb_ctrlrequest *crq)
{
	u8 ep_num = crq->wIndex & USB_ENDPOINT_NUMBER_MASK;
	u32 ep_ctrl;

	DEBUG_SETUP("%s: *** USB_REQ_GET_STATUS  \n",__func__);

	switch (crq->bRequestType & USB_RECIP_MASK) 
	{
		/* ref. USB spec. p255 */
		case USB_RECIP_INTERFACE:
			/* just return 0 */
			g_status = __constant_cpu_to_le16(0);
			DEBUG_SETUP("\tGET_STATUS: USB_RECIP_INTERFACE, g_stauts = %d\n", g_status);
			break;

		case USB_RECIP_DEVICE:
			/* return RemoteWakeup & SelfPowered */
			g_status = cpu_to_le16(dev->devstatus);
			DEBUG_SETUP("\tGET_STATUS: USB_RECIP_DEVICE, g_stauts = %d\n", g_status);
			break;

		case USB_RECIP_ENDPOINT:
			/* return state of halt */
			if (ep_num > S3C_MAX_ENDPOINTS || crq->wLength > 2) {
				DEBUG_ERROR("\tGET_STATUS: Not support EP or wLength\n");
				return -1;
			}

			g_status = __constant_cpu_to_le16(0);
			if (dev->ep[ep_num].stopped)				
				g_status = __constant_cpu_to_le16(1);

			DEBUG_SETUP("\tGET_STATUS: USB_RECIP_ENDPOINT[ep(%d).stopped]: g_stauts = %d\n", ep_num, g_status);

			if(ep_is_in(&(dev->ep[ep_num]))) {
				DEBUG_SETUP("\t DIEPCTL[%d] = 0x%x\n", ep_num, readl(S3C_UDC_OTG_DIEPCTL(ep_num)));				
			}
			else {
				DEBUG_SETUP("\t DOEPCTL[%d] = 0x%x\n", ep_num, readl(S3C_UDC_OTG_DOEPCTL(ep_num)));
			}

			break;

		default:
			return -1;
	}

	prefetch(&g_status);
	dma_cache_maint(&g_status, 2, DMA_TO_DEVICE);
	
	ep_ctrl = readl(S3C_UDC_OTG_DIEPCTL(EP0_CON));

#if OTG_DEDICATED_FIFO
	ep_ctrl = s3c_ep_tx_fifo_flush(ep_ctrl, EP0_CON);
#endif

	writel((1<<DEPTSIZ_PKT_CNT_BIT)|(2<<DEPTSIZ_XFER_SIZE_BIT), S3C_UDC_OTG_DIEPTSIZ(EP0_CON));
	writel(virt_to_phys(&g_status), S3C_UDC_OTG_DIEPDMA(EP0_CON));
	writel(ep_ctrl|DEPCTL_EPENA|DEPCTL_CNAK, S3C_UDC_OTG_DIEPCTL(EP0_CON));

	dev->ep0state = WAIT_FOR_SETUP;

	return 0;
}

/**
 * s3c_ep0_read
 * read a transfer at OUT data transaction of control transfer
 */
static void s3c_ep0_read(struct s3c_udc *dev)
{
	struct s3c_request *req;
	struct s3c_ep *ep = &dev->ep[0];
	int ret;

	if (!list_empty(&ep->queue))
	{
		req = list_entry(ep->queue.next, struct s3c_request, queue);
	}
	else 
	{
		DEBUG_ERROR("[%s] reading data from empty ep->queue\n", __func__);
		return;
	}

	DEBUG_EP0("[%s] req.length = 0x%x, req.actual = 0x%x\n",
		__func__, req->req.length, req->req.actual);

	if(req->req.length == 0) 
	{
		dev->ep0state = WAIT_FOR_SETUP;
		s3c_req_done(ep, req, 0);
		return;
	}

	ret = s3c_udc_set_dma_rx(ep, req);

	if (ret) 
	{
		dev->ep0state = WAIT_FOR_SETUP;
		s3c_req_done(ep, req, 0);
		return;
	}

}
//---------------------------------------------------------------------------------------

/**
 * s3c_ep0_write
 * write a transfer at IN data transaction of control transfer
 */
static int s3c_ep0_write(struct s3c_udc *dev)
{
	struct s3c_request *req;
	struct s3c_ep *ep = &dev->ep[0];
	int ret; 

	DEBUG_EP0("[%s] ep0 write\n", __func__);

	if (list_empty(&ep->queue))
	{
		req = 0;
	}
	else
	{
		req = list_entry(ep->queue.next, struct s3c_request, queue);
	}

	if (!req) 
	{
		DEBUG_EP0("[%s] NULL REQ\n", __func__);
		return 0;
	}

	DEBUG_EP0("[%s] req.length = 0x%x, req.actual = 0x%x\n", __func__, req->req.length, req->req.actual);

	

/*	req->zlp replaced need_zlp function */
#if 0
	int  need_zlp = 0;

	if (req->req.length - req->req.actual == ep->ep.maxpacket) 
	{
		/* Next write will end with the packet size, */
		/* so we need Zero-length-packet */
		need_zlp = 1;
	}
#endif	

	ret = s3c_ep0_write_fifo(ep, req);

	//if ((ret == 1) && !need_zlp) 
	if (ret == 1) 
	{
		/* Last packet */
		DEBUG_EP0("[%s] finished, waiting for status\n", __func__);
		dev->ep0state = WAIT_FOR_SETUP;
	} 
	else 
	{
		DEBUG_EP0("[%s] not finished\n", __func__);
		dev->ep0state = DATA_STATE_XMIT;
	}
#if 0
	if (need_zlp) {
		DEBUG_EP0("[%s] Need ZLP!\n", __func__);
		dev->ep0state = DATA_STATE_NEED_ZLP;
	}
#endif	
	return 1;
}
//---------------------------------------------------------------------------------------
#define S3C_EP0_STALL	(-2)

static int s3c_ep0_setup_handle_feature(struct s3c_udc *dev, struct usb_ctrlrequest ctrl_req)
{
	struct s3c_ep *ep;
	u8 ep_num;
	u32 uTemp ;

	/* returning '-1' to delete gadget_setup */
	/* returning S3C_EP0_STALL to make EP0 STALL state*/
	if ((ctrl_req.bRequestType & USB_TYPE_MASK) != (USB_TYPE_STANDARD))
	{
		DEBUG_FEATURE("[%s] bRequestType != (USB_DIR_IN | USB_TYPE_STANDARD) : delegated !!!	\n",__func__);
		return -1;
	}
	
	if ((ctrl_req.bRequestType & USB_RECIP_MASK) != USB_RECIP_ENDPOINT 
		&& (ctrl_req.bRequestType & USB_RECIP_MASK) != USB_RECIP_DEVICE )
	{
		DEBUG_FEATURE("[%s] bRequestType != USB_RECIP : delegated !!!  \n",__func__);
		return -1;
	}
				
	if (ctrl_req.bRequest == USB_REQ_CLEAR_FEATURE)
		DEBUG_FEATURE("USB_REQ_CLEAR_FEATURE\n");
	else
		DEBUG_FEATURE("USB_REQ_SET_FEATURE\n");

	if ((ctrl_req.bRequestType & USB_RECIP_MASK) == USB_RECIP_ENDPOINT )
	{
		DEBUG_FEATURE("\tUSB_RECIP_ENDPOINT\n");
		ep_num = ctrl_req.wIndex & USB_ENDPOINT_NUMBER_MASK;	
		ep = &dev->ep[ep_num];
		
		if (ctrl_req.wValue != 0 || ctrl_req.wLength != 0
			|| ep_num > S3C_MAX_ENDPOINTS || ep_num < 1)
		{
			DEBUG_ERROR("[%s] ctrl_req.wValue != 0 || ctrl_req.wLength != 0 || not support ep_num[%d]  \n",__func__, ep_num);
			return S3C_EP0_STALL;
		}
		//add ep descriptor for exit
		
		if (ctrl_req.wValue == USB_ENDPOINT_HALT) 
		{
			DEBUG_FEATURE("\tUSB_ENDPOINT_HALT\n");
			/* USB_REQ_CLEAR_FEATURE */
			if (ctrl_req.bRequest == USB_REQ_CLEAR_FEATURE)
			{
				if (ep_num == 0) 
					s3c_ep0_set_stall(ep);
				else
				{
					s3c_ep_clear_stall(ep);
					s3c_ep_activate(ep);
					ep->stopped = 0;						
					clear_feature_num = ep_num;
					clear_feature_flag = 1;
				}	
			}
			/* USB_REQ_SET_FEATURE */
			else
			{
				if (ep_num == 0) 
					s3c_ep0_set_stall(ep);
				else
				{
					//add deactivate
					ep->stopped = 1;
					s3c_ep_set_stall(ep);
				}
			}
		}
		else
		{
			DEBUG_ERROR("\tnot support wValue[%d]\n", ctrl_req.wValue);
			return S3C_EP0_STALL;
		}
	}
	else if((ctrl_req.bRequestType & USB_RECIP_MASK) == USB_RECIP_DEVICE )
	{
		DEBUG_FEATURE("\tUSB_RECIP_DEVICE\n");
		switch (ctrl_req.wValue) 
		{
			case USB_DEVICE_REMOTE_WAKEUP:
				DEBUG_FEATURE("\tUSB_DEVICE_REMOTE_WAKEUP\n");	
				if (ctrl_req.bRequest == USB_REQ_CLEAR_FEATURE)
					dev->devstatus &= ~(1 << USB_DEVICE_REMOTE_WAKEUP);
				else
					dev->devstatus |= 1 << USB_DEVICE_REMOTE_WAKEUP;
				break;
			
			/* (wired high speed only) */
			case USB_DEVICE_TEST_MODE:
				/* USB_REQ_SET_FEATURE */
				if (ctrl_req.bRequest != USB_REQ_CLEAR_FEATURE)
				{
					if((ctrl_req.wIndex&0xFF) != 0)
					{
						DEBUG_ERROR("\tThe lower byte of wIndex must be zero [wIndex = %d]\n", ctrl_req.wIndex);					
						return S3C_EP0_STALL;
						//return -1;
					}
					/*  
					 * the "Status" phase of the control transfer
					 * completes before transmitting the TEST packets. 
					 */
					s3c_ep_send_zlp(EP0_CON);
					
					uTemp = readl(S3C_UDC_OTG_DCTL);
					uTemp &= ~(0x70);
					switch (ctrl_req.wIndex >> 8) 
					{
						case 1: // TEST_J
							DEBUG_FEATURE("\tTEST_J\n");
							uTemp = (1<<4);
							break;

						case 2: // TEST_K	 
							DEBUG_FEATURE("\tTEST_K\n");
							uTemp = (2<<4);
							break;

						case 3: // TEST_SE0_NAK
							DEBUG_FEATURE("\tTEST_SE0_NAK\n");
							uTemp = (3<<4);
							break;

						case 4: // TEST_PACKET
							DEBUG_FEATURE("\tTEST_PACKET\n");
							uTemp = (4<<4);
							break;

						case 5: // TEST_FORCE_ENABLE
							DEBUG_FEATURE("\tTEST_FORCE_ENABLE\n");
							uTemp = (5<<4);
							break;
					}
					writel(uTemp, S3C_UDC_OTG_DCTL);
					return USB_DEVICE_TEST_MODE;
				}
				/* USB_REQ_CLEAR_FEATURE */
				else 
				{
			/* Test_mode feature cannot be cleared by the ClearFeature() requested */
					DEBUG_ERROR("\tTest_mode feature cannot be cleared by the ClearFeature()\n");
					//might be stall
					return S3C_EP0_STALL;
//					return -1;
				}
				break;
			
			case USB_DEVICE_B_HNP_ENABLE:
				DEBUG_FEATURE("\tNot yet Implemented : USB_DEVICE_B_HNP_ENABLE\n");
				return S3C_EP0_STALL;
				break;
			
			case USB_DEVICE_A_HNP_SUPPORT:
				/* RH port supports HNP */
				DEBUG_FEATURE("\tNot yet Implemented : USB_DEVICE_A_HNP_SUPPORT\n");
				return S3C_EP0_STALL;
				break;
			
			case USB_DEVICE_A_ALT_HNP_SUPPORT:
				/* other RH port does */
				DEBUG_FEATURE("\tNot yet Implemented : USB_DEVICE_A_ALT_HNP_SUPPORT\n");
				return S3C_EP0_STALL;
				break;
			default:
				DEBUG_ERROR("\tnot support wValue[0x%x]\n", ctrl_req.wValue);
				return S3C_EP0_STALL;
				break;
		}// switch
	}// if
	else {		
		DEBUG_ERROR("\t Must not come here !! \n");
		return S3C_EP0_STALL;
	}
	return 0;
}

#if ENABLE_WA_MISSING_SETUP
int missing_clear_feature = 0;
int missing_set_address = 0;
int old_address = 0;
int temp_acm = 0;
#endif

/*
 * s3c_ep0_setup
 * handle of setup transaction data during WAIT_FOR_SETUP 
 */
static void s3c_ep0_setup(struct s3c_udc *dev)
{
	struct s3c_ep *ep = &dev->ep[0];
	int i= 0, is_in; 
	int temp_ret;

	/* Nuke all previous transfers */
	s3c_ep_nuke(ep, -EPROTO);
	
	/*	
	 *	w/o prefetchw & dma_cache_maint
	 *	there should be a timing promblem at acm connection 
	 *	refer [s3c_ep0_setup] CHANGE g_ctrl.wIndex = 0x82 => 0
	 */
	prefetchw(&g_ctrl);
	dma_cache_maint(&g_ctrl, sizeof(struct usb_ctrlrequest), DMA_FROM_DEVICE);

	DEBUG_SETUP("Read CTRL REQ 8 bytes\n");
	DEBUG_SETUP("  CTRL.bRequestType = 0x%x (direction %s)\n", g_ctrl.bRequestType,
		    g_ctrl.bRequestType & USB_DIR_IN ? "IN" :"OUT");
	DEBUG_SETUP("  CTRL.bRequest = 0x%x\n", g_ctrl.bRequest);
	DEBUG_SETUP("  CTRL.wLength = 0x%x\n", g_ctrl.wLength);
	DEBUG_SETUP("  CTRL.wValue = 0x%x (%d)\n", g_ctrl.wValue, g_ctrl.wValue >> 8);
	DEBUG_SETUP("  CTRL.wIndex = 0x%x\n", g_ctrl.wIndex);

#if ENABLE_WA_MISSING_SETUP
	if(temp_acm == 1) {		
		temp_acm = 0;		
		if(g_ctrl.bRequestType == 0 && g_ctrl.bRequest == 0xc2 ) {			
			DEBUG_ERROR("[%s] ERROR: missing USB_CDC_REQ_SET_LINE_CODING setup packet\n", __func__);
			DEBUG_ERROR("[%s] got g_ctrl.bRequestType == 0 && g_ctrl.bRequest == 0xc2 \n", __func__);
		}
	}

	// only for cygnus
	// sw walk around (improper reading string request instead of clear_feature host sent)
	if(g_ctrl.bRequestType == 0x80 && g_ctrl.bRequest == 0x06 && 
		g_ctrl.wValue == 0x0303 && g_ctrl.wIndex == 0x0409) {

		//get_descriptor for string size
		if(g_ctrl.wLength == 2){
			missing_clear_feature = 0;
		}
		//get_descriptor for real string
		if(g_ctrl.wLength == 26){			
			if (missing_clear_feature == 1) {				
				DEBUG_ERROR("[adonis] SW walk-around (improper reading string request instead of clear_feature host sent)\n");				
				DEBUG_ERROR("[adonis] changing get string to clear_feature \n");				
				g_ctrl.bRequestType = 0x02;
				g_ctrl.bRequest = 0x01;
				g_ctrl.wLength = 0;
				g_ctrl.wValue = 0;
				g_ctrl.wIndex = 0x82;
			}
			missing_clear_feature = 1;
		}		
	}	
		
	//only for cygnus
	// sw walk around (improper reading get descriptor instead of set_address host sent)
	if(dev->udc_state < USB_STATE_ADDRESS && g_ctrl.bRequestType == 0x80 && 
		g_ctrl.bRequest == 0x06 && g_ctrl.wValue == 0x0100 && g_ctrl.wIndex == 0) {	
		//get_descriptor for string size
		if(g_ctrl.wLength == 64){
			missing_set_address = 0;
		}
		//get_descriptor for real string
		if(g_ctrl.wLength == 18){			
			if (missing_set_address == 1) {				
				DEBUG_ERROR("[adonis] SW walk-around (improper reading get descriptor instead of set_address host sent)\n");								
				g_ctrl.bRequestType = 0;
				g_ctrl.bRequest = 0x05;
				g_ctrl.wLength = 0;
				g_ctrl.wValue = old_address;
				g_ctrl.wIndex = 0;
			}
			missing_set_address = 1;
		}		
	}
#endif
		
	/* Set direction of EP0 */
	if (likely(g_ctrl.bRequestType & USB_DIR_IN)) 
	{
		ep->bEndpointAddress |= USB_DIR_IN;
		is_in = 1;
	} 
	else 
	{
		ep->bEndpointAddress &= ~USB_DIR_IN;
		is_in = 0;
	}
	
/*
 *	the following is dedicated to class type request
 *	should make some logic for req->length == 0 , status w/o data stage
 */
	// Confirm CLASS TYPE request
	if ( (g_ctrl.bRequestType & USB_TYPE_MASK) == USB_TYPE_CLASS ) 
	{ 
		DEBUG_SETUP("CLASS Type request. ep->bEndpointAddress : 0x%02x\n", ep->bEndpointAddress);
		 
		switch(g_ctrl.bRequest)
		{
/* 
 *	In case of serial class type such as g_serial, cdc, acm
 *  in include/linux/usb/cdc.h
 */
#define USB_CDC_REQ_SET_LINE_CODING			0x20
#define USB_CDC_REQ_GET_LINE_CODING			0x21
#define USB_CDC_REQ_SET_CONTROL_LINE_STATE	0x22
			case USB_CDC_REQ_SET_LINE_CODING :
				DEBUG_SETUP("USB_CDC_REQ_SET_LINE_CODING\n");				
				ep->bEndpointAddress &= ~USB_DIR_IN;				
#if ENABLE_WA_MISSING_SETUP
				temp_acm = 1;
#endif
				//read more 7 bytes data
				goto gadget_setup;
			case USB_CDC_REQ_GET_LINE_CODING :
				DEBUG_SETUP("USB_CDC_REQ_GET_LINE_CODING\n");
				DEBUG_SETUP("modify USB_DIR_IN\n");
				ep->bEndpointAddress |= USB_DIR_IN;
				goto gadget_setup;
			case USB_CDC_REQ_SET_CONTROL_LINE_STATE :				
				DEBUG_SETUP("USB_CDC_REQ_SET_CONTROL_LINE_STATE\n");
				DEBUG_SETUP("modify USB_DIR_IN\n");
				ep->bEndpointAddress |= USB_DIR_IN;

				if (dev->udc_state < USB_STATE_CONFIGURED) {			
					DEBUG_ERROR("[adonis] Not proper Host's request udc hasn't yet configured \n");
					DEBUG_ERROR("[adonis] SW walk-around just return \n");					
					return;
				}
		//changing wIndex for Interface number
		//ACM interface ID is 0
				if( (dev->config_gadget_driver == ANDROID_ADB_UMS_ACM && g_ctrl.wIndex != 0)||
					(dev->config_gadget_driver == ANDROID_ADB_UMS_ACM_ETH_RNDIS	&& g_ctrl.wIndex != 4) )
				{			
					DEBUG_ERROR("\n\n[%s] CHANGE g_ctrl.wIndex = 0x%x => 0\n\n",
						__func__, g_ctrl.wIndex);
					g_ctrl.wIndex = 0;
				}

				if(dev->config_gadget_driver == ANDROID_ADB_UMS_ACM_ETH_RNDIS_MTP)
				{
					//writel( readl(S3C_UDC_OTG_DOEPCTL(1)) &~(DEPCTL_USBACTEP), S3C_UDC_OTG_DOEPCTL(1)); 					
					//printk("  CTRL.wValue = 0x%x (%d)\n", le16_to_cpu(g_ctrl.wValue), le16_to_cpu(g_ctrl.wValue) >> 8);					
				
					if(tmp_1st == false && g_ctrl.wValue == 0x03)
					{
						DEBUG_ERROR("[USB_CDC_REQ_SET_CONTROL_LINE_STATE] : g_ctrl.wValue == 0x%x (%d)\n", le16_to_cpu(g_ctrl.wValue), le16_to_cpu(g_ctrl.wValue) >> 8);
						DEBUG_ERROR("USB EP1 OUT disable DEPCTL_USBACTEP \n");
						writel( readl(S3C_UDC_OTG_DOEPCTL(EP1_OUT)) &~(DEPCTL_USBACTEP), S3C_UDC_OTG_DOEPCTL(EP1_OUT)); 
						tmp_1st = true;
					}
					if(g_ctrl.wValue == 0x00)
					{
					//	printk("[USB_CDC_REQ_SET_CONTROL_LINE_STATE] : g_ctrl.wValue == 0x00 \n");
					//	writel(readl(S3C_UDC_OTG_DOEPCTL(1)) | (DEPCTL_USBACTEP), S3C_UDC_OTG_DOEPCTL(1)); 
					}
				}
				
				goto gadget_setup;
/* for g_cdc */
#define USB_CDC_SET_ETHERNET_PACKET_FILTER	0x43
			case USB_CDC_SET_ETHERNET_PACKET_FILTER :				
				DEBUG_SETUP("USB_CDC_SET_ETHERNET_PACKET_FILTER\n");
				DEBUG_SETUP("modify USB_DIR_IN\n");
				ep->bEndpointAddress |= USB_DIR_IN;
				goto gadget_setup;
		}
	}

	/* Handle some standard request SETUP packets to udc*/
	switch (g_ctrl.bRequest) {
		case USB_REQ_CLEAR_FEATURE:
		case USB_REQ_SET_FEATURE:
			temp_ret = s3c_ep0_setup_handle_feature(dev, g_ctrl);
			if( temp_ret == USB_DEVICE_TEST_MODE)
				return;
			else if (temp_ret == -1)
				goto gadget_setup;			
			else if (temp_ret == S3C_EP0_STALL)
				goto gadget_stall;
			s3c_ep_send_zlp(EP0_CON);
			return;

		case USB_REQ_SET_ADDRESS:
			if (g_ctrl.bRequestType != (USB_TYPE_STANDARD | USB_RECIP_DEVICE))
				break;
			printk("[%s] USB_REQ_SET_ADDRESS (%d)\n", dev->driver->driver.name, g_ctrl.wValue);
			s3c_udc_set_address(dev, g_ctrl.wValue);
			s3c_ep_send_zlp(EP0_CON);
			
#if ENABLE_WA_MISSING_SETUP
			old_address = g_ctrl.wValue;
#endif
			return;

		case USB_REQ_SET_CONFIGURATION :
			DEBUG_SETUP("[%s] USB_REQ_SET_CONFIGURATION (%d)\n",
					__func__, g_ctrl.wValue);
			/* gadget will queue zlp */
			//CHECK modify direction
			DEBUG_SETUP("modify USB_DIR_IN\n");
			ep->bEndpointAddress |= USB_DIR_IN;
		
			/* g_ether's CDC config => 1, RNDIS => 2 */
			if (strcmp(dev->driver->driver.name, "g_ether") == 0 )
			{
				switch(g_ctrl.wValue )
				{
					case 1:	//CDC
						dev->config_gadget_driver = ETHER_CDC;
						break;
					case 2: //RNDIS
						dev->config_gadget_driver = ETHER_RNDIS;
						break;
					default:
						DEBUG_ERROR("[%s]not proper g_ctrl.wValue[%d]\n", __func__, g_ctrl.wValue );
				}
			}
			dev->udc_state = USB_STATE_CONFIGURED;

			if (g_ctrl.wValue == 0) {	
				/* prevent new request submissions, kill any outstanding requests  */
				for (i = 0; i <= S3C_MAX_ENDPOINTS; i++) 
				{
					ep = &dev->ep[i];
					ep->stopped = 1;
					s3c_ep_nuke(ep, -ESHUTDOWN);
				}
								
				/* re-init driver-visible data structures */
				s3c_ep_list_reinit(dev);		
				zero_config_flag = 1;
			}
			
			break;

		case USB_REQ_GET_DESCRIPTOR:
			DEBUG_SETUP("[%s] *** USB_REQ_GET_DESCRIPTOR  \n",__func__);
			/*
			 * it only happend at first time connection.
			 * Host send a packet with wValue has 0x03EE
			 * 03 is for String Type & EE is a String ID 
			 * 0xEE is for Microsoft OS Descriptor.
			 * if not support, response with STALL
			 */
#if 0
			if (g_ctrl.wValue == 0x03ee) 
			{
				DEBUG_SETUP("[%s]Not support Microsoft OS String\n", __func__);
				goto gadget_stall;
			}
#endif
			break;
			
		case USB_REQ_SET_INTERFACE:
			//confirm USB_RECIP_INTERFACE?
			if ((g_ctrl.bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE)
				break;
			DEBUG_SETUP("[%s] *** USB_REQ_SET_INTERFACE [wIndex = %d] <= wValue (%d)\n",
				__func__, g_ctrl.wIndex, g_ctrl.wValue);	
			/* gadget will queue zlp */
			//CHECK modify direction
			DEBUG_SETUP("modify USB_DIR_IN\n");
			ep->bEndpointAddress |= USB_DIR_IN;
	/* 
	 * [SW Walk-around] Only for USBCV's MSC test scenario
	 * USB_REQ_SET_INTERFACE of UMS miss INQUIRY after USB_REQ_SET_CONFIGURATION
	 * if UMS already set by USB_REQ_SET_CONFIGURATION(1)
	 * following zero configuration
	 * USB_REQ_SET_INTERFACE return OK by sending zlp 
	 * without real handling USB_REQ_SET_INTERFACE of UMS
	 */
			if( (dev->config_gadget_driver == ANDROID_ADB_UMS_ACM ||
				dev->config_gadget_driver == ANDROID_ADB_UMS_ACM_ETH_RNDIS)&&
				g_ctrl.wIndex == 2 && zero_config_flag == 1) {
				DEBUG_ERROR("\n\n[%s] [SW Walk-around] USBCV's MSC test\n",__func__);
				zero_config_flag = 0;
				s3c_ep_send_zlp(EP0_CON);
				return;
			}

			/* 
			 * [SW Walk-around] Only for USB CV's ch9 test scenario
			 * RNDIS gadget late responds @ continuous USB_REQ_SET_CONFIGURATION & USB_REQ_SET_INTERFACE request scenario
			 * that only occured on USB CV test scenario
			 * Because of late response, RNDIS tries to enable EP with NULL descriptor pointer making USB CV test failed
			 * Sending zlp @ USB_REQ_SET_INTERFACE of interface #1 doesn't delegate this request to RNDIS gadget
			 */
			if( dev->config_gadget_driver == ANDROID_ADB_UMS_ACM_ETH_RNDIS_MTP && 
				g_ctrl.wIndex == 1) {
				DEBUG_ERROR("\n\n[%s] [SW Walk-around] USBCV's test , [Send ZLP] @ USB_REQ_SET_INTERFACE\n",__func__);
				s3c_ep_send_zlp(EP0_CON);
				return;
			}

			break;
				
		case USB_REQ_GET_CONFIGURATION:
			DEBUG_SETUP("[%s] *** USB_REQ_GET_CONFIGURATION  \n",__func__);
			break;

		case USB_REQ_GET_STATUS:
			DEBUG_SETUP("[%s] *** USB_REQ_GET_STATUS  \n",__func__);

			if ((g_ctrl.bRequestType & (USB_DIR_IN | USB_TYPE_MASK))
					!= (USB_DIR_IN | USB_TYPE_STANDARD))
			{
				DEBUG_SETUP("[%s] *** USB_REQ_GET_STATUS : delegated !!!  \n",__func__);
					break;
			}

			if (!s3c_udc_get_status(dev, &g_ctrl)) 
			{
				return;
			}
			break;
		case USB_REQ_SYNCH_FRAME:
			DEBUG_SETUP("[%s] *** USB_REQ_SYNCH_FRAME  \n",__func__);
			break;
			
		default:
			DEBUG_SETUP("[%s] Default of g_ctrl.bRequest=0x%x happened.\n",
					__func__, g_ctrl.bRequest);
			break;
	}// switch

gadget_setup:
	if (likely(dev->driver)) 
	{
		/* device-2-host (IN) or no data setup command,
		 * process immediately */
		S3C_UDC_UNLOCK(&dev->lock);
		DEBUG_SETUP("[%s] usb_ctrlrequest will be passed to gadget's setup()\n", __func__);
		i = dev->driver->setup(&dev->gadget, &g_ctrl);
		S3C_UDC_LOCK(&dev->lock);

		if (i < 0) 
		{
gadget_stall:
			/* setup processing failed, force stall */
			DEBUG_ERROR("[%s] gadget setup FAILED (stalling EP 0), setup returned %d\n",
				__func__, i);
			s3c_ep0_set_stall(&dev->ep[0]);
		}
	}
}
//---------------------------------------------------------------------------------------

/*
 * s3c_handle_ep
 * handling EP0's OUT Token for setup transaction of Control transfer 
 */
static void s3c_ep0_handle(struct s3c_udc *dev)
{
	if (dev->ep0state == WAIT_FOR_SETUP) 
	{
		DEBUG_EP0("[%s] WAIT_FOR_SETUP\n", __func__);
		s3c_ep0_setup(dev);
	} 
	else 
	{
		DEBUG_EP0("[%s] strange state!!(state = %s)\n",
			__func__, state_names[dev->ep0state]);
	}
}
//---------------------------------------------------------------------------------------

