#ifndef ns_mnc_prj_h
#define ns_mnc_prj_h

#include "address.h"
#include "ip.h"

class MncPrjSendTimer;
class MncPrjRecvTimer;

class EventTrace;

class MncPrj : public Mac {

	friend class BackoffTimer;
public:
	MncPrj();
	void recv(Packet *p, Handler *h);
	void send(Packet *p, Handler *h);

	void sendHandler(void);
	void recvHandler(void);
	double txtime(Packet *p);

	void trace_event(char *, Packet *);
	int command(int, const char*const*);
	EventTrace *et_;
private:
	Packet *	pktRx_;
	Packet *	pktTx_;
        MacState        rx_state_;      // incoming state (MAC_RECV or MAC_IDLE)
	MacState        tx_state_;      // outgoing state
        int             tx_active_;
	int             fullduplex_mode_;
	Handler * 	txHandler_;

	MncPrjSendTimer *sendTimer;
	MncPrjRecvTimer *recvTimer;
	int busy_ ;
	int repeatTx_;
	double cbrInterval_;
	double randTimes_[10];
	int count;

};

class MncPrjTimer: public Handler {
public:
	MncPrjTimer(MncPrj* m) : mac(m) {
	  	busy_ = 0;
	}
	virtual void handle(Event *e) = 0;
	virtual void restart(double time);
	virtual void start(double time);
	virtual void stop(void);
	inline int busy(void) { return busy_; }
	inline double expire(void) {
		return ((stime + rtime) - Scheduler::instance().clock());
	}

protected:
	MncPrj	*mac;
	int		busy_;
	Event		intr;
	double		stime;
	double		rtime;
	double		slottime;
};


//  Timer to use for finishing sending of packets
class MncPrjSendTimer: public MncPrjTimer {
public:
	MncPrjSendTimer(MncPrj *m) : MncPrjTimer(m) {}
	void handle(Event *e);
};

// Timer to use for finishing reception of packets
class MncPrjRecvTimer: public MncPrjTimer {
public:
	MncPrjRecvTimer(MncPrj *m) : MncPrjTimer(m) {}
	void handle(Event *e);
};

#endif
