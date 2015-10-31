use Test::More tests => 26;
use IPC::Run qw(start);

my $ihlt = start(['src/ihlt','-f'],undef,'>&2');

use IO::Socket::INET;

# create a connecting socket
my @sockets;
for(1..3) {
    my $ctr = 0;
    do {
	sleep 3 + $ctr;
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

$sockets[2]->autoflush(1);
ok $sockets[2]->send("wall Hello World!") == 17, 'wall msg began';

# data to send to a server
$sockets[1]->autoflush(1);
ok $sockets[1]->send("wall Hello World!\n") == 18, 'wall msg sent';
$sockets[2]->recv($response, 28);
ok $response eq "211 wall: Hello World!\r\n", 'wall msg recv';
$sockets[3]->recv($response, 28);
ok $response eq "211 wall: Hello World!\r\n", 'wall msg recv again';

$sockets[3]->autoflush(1);
ok $sockets[3]->send("next Hello World!\n") == 18, 'next msg sent';
$sockets[1]->recv($response, 28);
ok $response eq "211 prev: Hello World!\r\n", 'next msg recv';

ok $sockets[1]->send("prev Hello World!\n") == 18, 'prev msg sent';
$sockets[3]->recv($response, 28);
ok $response eq "211 next: Hello World!\r\n", 'prev msg recv';

ok $sockets[3]->send("ping Hello World!\n") == 18, 'echo msg sent';
$sockets[3]->recv($response, 28);
ok $response eq "211 pong: Hello World!\r\n", 'echo msg recv';

ok $sockets[1]->send("bad Hello World!\n") == 17, 'bad msg sent';
$sockets[1]->recv($response, 53);
ok $response eq "502 Bad command or it is not implemented here.\r\n", 'bad msg recv';

$sockets[1]->blocking(0);
$sockets[1]->recv($response, 28);
$sockets[1]->blocking(1);
ok $! == $!{EWOULDBLOCK}, 'recv would block';
ok $response eq "", 'recv string empty';
$sockets[3]->blocking(0);
$sockets[3]->recv($response, 28);
$sockets[3]->blocking(1);
ok $! == $!{EWOULDBLOCK}, 'recv would block again';
ok $response eq "", 'recv string empty again';

ok $sockets[2]->send("\n") == 1, 'wall msg sent';
$sockets[1]->recv($response, 28);
ok $response eq "211 wall: Hello World!\r\n", 'wall msg recv';
$sockets[3]->recv($response, 28);
ok $response eq "211 wall: Hello World!\r\n", 'wall msg recv';

ok $sockets[2]->send("quit\r\n") == 6, 'sent quit';

ok $sockets[$_]->close(), "$_: closed" for(1..3);

exit 0;
