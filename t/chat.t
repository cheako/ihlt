use Test::More tests => 12;
use IPC::Run qw(start);

my $ihlt = start(['src/ihlt'],undef,'>&2');
END { $ihlt->kill_kill; }

sleep 3;
use IO::Socket::INET;

# create a connecting socket
my @sockets;
for(1..3) {
    my $ctr = 0;
    do {
	sleep $ctr;
        $sockets[$_] = new IO::Socket::INET(
            PeerHost => '127.0.0.1',
            PeerPort => '4458',
            Proto    => 'tcp',
        );
      } while ( !$socket[$_]
        && $_ == 1
        && $! == $!{ECONNREFUSED}
        && $ctr++ < 30);
    unless ( $sockets[$_] ) {
        fail "$_: $ctr: cannot connect to the server $!\n";
        die;
    }
    pass"$_: connected to the server";
}

my $response = "";
sleep 3;

# data to send to a server
$sockets[1]->autoflush(1);
ok $sockets[1]->send("Hello World!\n") == 13, 'first msg sent';
$sockets[2]->recv($response, 20);
ok $response eq "Hello World!\n", 'first msg recv';
$sockets[3]->recv($response, 20);
ok $response eq "Hello World!\n", 'first msg recv again';

$sockets[3]->autoflush(1);
ok $sockets[3]->send("Hello World!\n") == 13, 'second msg sent';
$sockets[2]->recv($response, 20);
ok $response eq "Hello World!\n", 'second msg recv';
$sockets[1]->recv($response, 20);
ok $response eq "Hello World!\n", 'second msg recv again';

ok $sockets[$_]->close(), "$_: closed" for(1..3);

$ihlt->kill_kill;
END { }
exit 0;
