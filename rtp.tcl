set cbr_size 128.0
set cbr_interval 0.02
set time_duration 100


# ======================================================================
# Define options
# ======================================================================

set val(chan) Channel/WirelessChannel ;# channel type
set val(prop) Propagation/TwoRayGround ;# radio-propagation model
set val(netif) Phy/WirelessPhy ;# network interface type
set val(mac) Mac/MncPrj ;# MAC type
set val(ifq) Queue/DropTail/PriQueue ;# interface queue type
set val(ll) LL ;# link layer type
set val(ant) Antenna/OmniAntenna ;# antenna model
set val(ifqlen) 5000 ;# max packet in ifq
set val(rp) DumbAgent ;# routing protocol
set val(repeatTimes) 2;
set val(fullduplexMode) 1;
set val(cbr_size) 128.0;

$val(mac) set fullduplex_mode_ $val(fullduplexMode);
$val(mac) set repeatTx $val(repeatTimes);
$val(mac) set cbrInterval $cbr_interval;

#Set variables to be used for topology object
set val(x) 50
set val(y) 50
set val(z) 50

#Create a simulator object
set ns [new Simulator]

#Set tracing on
set tracefile [open simulation.tr w]
$ns trace-all $tracefile

# Set up a topography object
set topol [new Topography]
$topol load_flatgrid $val(x) $val(y)

# define some more variables
set val(nn) 101; # number of mobile nodes

# create the GOD object
create-god $val(nn)

# create the channel used for propagation
set channel [new Channel/WirelessChannel]

# configure node

$ns node-config -adhocRouting $val(rp) \
			-llType $val(ll) \
			-macType $val(mac) \
			-ifqType $val(ifq) \
			-ifqLen $val(ifqlen) \
			-antType $val(ant) \
			-propType $val(prop) \
			-phyType $val(netif) \
			-channel $channel \
			-topoInstance $topol \
			-agentTrace ON \
			-routerTrace OFF\
			-macTrace ON \
			-movementTrace OFF \
			-energyModel EnergyModel \
			-idlePower 0.035 \
			-rxPower 0 \
			-txPower 0.660 \
			-sleepPower 0.001 \
			-transitionPower 0.05 \
			-transitionTime 0.005 \
			-initialEnergy 1000 \

# set up a random number generator
set rng [new RNG]
$rng seed 1

# and a random variable for node positioning
set randl [new RandomVariable/Uniform]
$randl use-rng $rng
$randl set min_ -25.0
$randl set max_ 25.0


# use the rv to assgin random positions to each mobile node
for {set i 0} {$i < $val(nn)} {incr i} {
	set node($i) [$ns node];
	$node($i) random-motion 0; # disable random motion
	$node($i) set X_ [expr 25 + [$randl value]];
	$node($i) set Y_ [expr 25 + [$randl value]];
	$ns initial_node_pos $node($i) 20
}

# one mobile node (say node0) is a sink
set sink [new Agent/LossMonitor]
$ns attach-agent $node(0) $sink

# create traffic sources for each node and attach
for {set i 1} {$i < $val(nn)} {incr i} {
	set protocol($i) [new Agent/UDP]
	$ns attach-agent $node($i) $protocol($i)
	$ns connect $protocol($i) $sink
	set traffic($i) [new Application/Traffic/CBR]
	$traffic($i) set packetSize_ $cbr_size
	$traffic($i) set interval_ $cbr_interval
	$traffic($i) attach-agent $protocol($i)
	$ns at 1 "$traffic($i) start"
}

set stats [open statistics.dat w]

# Function to collect stats on the losses in traffic flow
proc update_stats { } {
	global sink
	global stats
	global ns

	set bytes [$sink set bytes_]
	set now [$ns now]
	puts $stats "$now $bytes"

	#Reset the bytes_ value on the sink
	$sink set bytes_ 0

	# Reschedule the procedure
	set time 1
	$ns at [expr $now+$time] "update_stats"
}

# define a function to handle the cleanup when the simulation ends
proc finish { } {
	global ns tracefile
	$ns flush-trace
	close $tracefile
	exit 0
 }

# call the functions, start the simulation

$ns at 1 "update_stats"
$ns at $time_duration "finish"

$ns run
