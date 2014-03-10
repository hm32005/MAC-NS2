#include "ll.h"
#include "mac.h"
#include "mac-mncprj.h"
#include "random.h"

// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
#include "agent.h"
#include "basetrace.h"

#include "cmu-trace.h"


static class MncPrjClass : public TclClass {
public:
	MncPrjClass() : TclClass("Mac/MncPrj") {
	}
	TclObject* create(int, const char*const*) {
		
		return new MncPrj();
	}

} class_MncPrj;


// Added by Sushmita to support event tracing (singal@nunki.usc.edu).
void MncPrj::trace_event(char *eventtype, Packet *p)
{
	if (et_ == NULL) return;
	char *wrk = et_->buffer();
	char *nwrk = et_->nbuffer();

	hdr_ip *iph = hdr_ip::access(p);
	char *src_nodeaddr =
		Address::instance().print_nodeaddr(iph->saddr());
	char *dst_nodeaddr =
		Address::instance().print_nodeaddr(iph->daddr());

	if (wrk != 0)
	{
		sprintf(wrk, "E -t "TIME_FORMAT" %s %s %s",
			et_->round(Scheduler::instance().clock()),
			eventtype,
			src_nodeaddr,
			dst_nodeaddr);
	}
	if (nwrk != 0)
	{
		sprintf(nwrk, "E -t "TIME_FORMAT" %s %s %s",
		et_->round(Scheduler::instance().clock()),
		eventtype,
		src_nodeaddr,
		dst_nodeaddr);
	}
	et_->dump();
}

MncPrj::MncPrj() : Mac() {
	rx_state_ = tx_state_ = MAC_IDLE;
	tx_active_ = 0;

	sendTimer = new MncPrjSendTimer(this);
	recvTimer = new MncPrjRecvTimer(this);
	// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
	et_ = new EventTrace();
	busy_ = 0;
	bind("fullduplex_mode_", &fullduplex_mode_);

	Tcl& tcl = Tcl::instance();
	tcl.evalf("Mac/MncPrj set repeatTx");
	bind("repeatTx", &repeatTx_);

	tcl.evalf("Mac/MncPrj set cbrInterval");
	bind("cbrInterval", &cbrInterval_);
}

// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
int
MncPrj::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if(strcmp(argv[1], "eventtrace") == 0) {
			et_ = (EventTrace *)TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}
	return Mac::command(argc, argv);
}

void MncPrj::recv(Packet *p, Handler *h) {

	struct hdr_cmn *hdr = HDR_CMN(p);
	/* let MacSimple::send handle the outgoing packets */
	if (hdr->direction() == hdr_cmn::DOWN) {
		send(p,h);
		return;
	}

	/* handle an incoming packet */

	/*
	 * If we are transmitting, then set the error bit in the packet
	 * so that it will be thrown away
	 */
	
	// in full duplex mode it can recv and send at the same time
	if (!fullduplex_mode_ && tx_active_)
	{
		hdr->error() = 1;

	}

	/*
	 * check to see if we're already receiving a different packet
	 */
	
	if (rx_state_ == MAC_IDLE) {
		/*
		 * We aren't already receiving any packets, so go ahead
		 * and try to receive this one.
		 */
		rx_state_ = MAC_RECV;
		pktRx_ = p;
		/* schedule reception of the packet */
		recvTimer->start(txtime(p));
	} else {
		/*
		 * We are receiving a different packet, so decide whether
		 * the new packet's power is high enough to notice it.
		 */
		if (pktRx_->txinfo_.RxPr / p->txinfo_.RxPr >= p->txinfo_.CPThresh) {
			/* power too low, ignore the packet */
			Packet::free(p);
		} else {
			/* power is high enough to result in collision */
			rx_state_ = MAC_COLL;

			/*
			 * look at the length of each packet and update the
			 * timer if necessary
			 */

			if (txtime(p) > recvTimer->expire()) {
				recvTimer->stop();
				Packet::free(pktRx_);
				pktRx_ = p;
				recvTimer->start(txtime(pktRx_));
			} else {
				Packet::free(p);
			}
		}
	}
}


double
MncPrj::txtime(Packet *p)
 {
	 struct hdr_cmn *ch = HDR_CMN(p);
	 double t = ch->txtime();
	 if (t < 0.0)
	 	t = 0.0;
	 return t;
 }



void MncPrj::send(Packet *p, Handler *h)
{
	printf("Time  = %f\n", Scheduler::instance().clock());
	
	hdr_cmn* ch = HDR_CMN(p);

	// store data tx time
 	ch->txtime() = Mac::txtime(ch->size());

	pktTx_ = p;
	txHandler_ = h;

	

	// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
	trace_event("SENSING_CARRIER",p);

	// check whether we're idle
	if (tx_state_ != MAC_IDLE) {
		// already transmitting another packet .. drop this one
		// Note that this normally won't happen due to the queue
		// between the LL and the MAC .. the queue won't send us
		// another packet until we call its handler in sendHandler()

		Packet::free(p);
		return;
	}

	


	if(rx_state_ != MAC_IDLE) {
		trace_event("BACKING_OFF",p);
	}

	int i;
	RNG* rv = new RNG();
	for (i = 0; i < repeatTx_; i++){
		double newTime = rv->uniform(0, cbrInterval_);
		randTimes_[i] = newTime;

	}

	//sort times
	int j=0;
	for(i=0; i<repeatTx_; i++)
	{
		for(j=0; j<repeatTx_-1; j++)
		{
			if(randTimes_[j]>randTimes_[j+1])
			{
				double temp = randTimes_[j+1];
				randTimes_[j+1] = randTimes_[j];
				randTimes_[j] = temp;
			}
		}
	}
	
	
	sendTimer->restart(randTimes_[0]);
	count = 0;

}


void MncPrj::recvHandler()
{
	hdr_cmn *ch = HDR_CMN(pktRx_);
	Packet* p = pktRx_;
	MacState state = rx_state_;
	pktRx_ = 0;
	int dst = hdr_dst((char*)HDR_MAC(p));

	rx_state_ = MAC_IDLE;

	// in full duplex mode we can send and recv at the same time
	// as different chanels are used for tx and rx'ing
	if (!fullduplex_mode_ && tx_active_) {
		// we are currently sending, so discard packet
		Packet::free(p);
	} else if (state == MAC_COLL) {
		// recv collision, so discard the packet
		drop(p, DROP_MAC_COLLISION);
		//Packet::free(p);
	} else if (dst != index_ && (u_int32_t)dst != MAC_BROADCAST) {

		/*  address filtering
		 *  We don't want to log this event, so we just free
		 *  the packet instead of calling the drop routine.
		 */
		Packet::free(p);
	} else if (ch->error()) {
		// packet has errors, so discard it
		//Packet::free(p);
		drop(p, DROP_MAC_PACKET_ERROR);

	} else {
		uptarget_->recv(p, (Handler*) 0);
	}
}



void MncPrj::sendHandler()
{
	
	Handler *h = txHandler_;
	Packet *p = pktTx_;
	
	tx_state_ = MAC_SEND;
	tx_active_ = 1;
	Packet *copy = p->copy();
	downtarget_->recv(copy, h);

	copy = 0;
	tx_state_ = MAC_IDLE;
	tx_active_ = 0;

	count++;
	if(count<repeatTx_)
	{
		sendTimer->restart(randTimes_[count]-randTimes_[count-1]);	
	}
	else{
		pktTx_ = 0;
		txHandler_ = 0;
		
		h->handle(p);
	}	
}

//  Timers

void MncPrjTimer::restart(double time)
{
	if (busy_)
		stop();
	start(time);
}



void MncPrjTimer::start(double time)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_ == 0);

	busy_ = 1;
	stime = s.clock();
	rtime = time;
	assert(rtime >= 0.0);

	s.schedule(this, &intr, rtime);
}

void MncPrjTimer::stop(void)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_);
	s.cancel(&intr);

	busy_ = 0;
	stime = rtime = 0.0;
}

void MncPrjSendTimer::handle(Event *)
{
	busy_ = 0;
	stime = rtime = 0.0;

	mac->sendHandler();
}

void MncPrjRecvTimer::handle(Event *)
{
	busy_ = 0;
	stime = rtime = 0.0;

	mac->recvHandler();
}


