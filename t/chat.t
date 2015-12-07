use Test::More tests => 27;
use IPC::Run qw(start);

my $ihlt =
  start( [ 'valgrind', '--error-exitcode=63', 'ltrace', 'src/ihlt', '-f' ],
    \undef, '>&2' );

use IO::Socket::SSL;

# create a connecting socket
my @sockets;
for ( 1 .. 3 ) {
    my $ctr = 0;
    do {
        diag "Connection $_ try $ctr: $!"
          if ( $ctr != 0 );
        sleep( ( $_ == 1 && $ctr == 0 ) * 70 + $ctr );
        $sockets[$_] = new IO::Socket::SSL(
            PeerHost => '127.0.0.1',
            PeerPort => '4458',
            Proto    => 'tcp',
        );
      } while ( !$socket[$_]
        && $_ == 1
        && $! == $!{ECONNREFUSED}
        && $ctr++ < 4 );
    unless ( $sockets[$_] ) {
        fail "$_: $ctr: cannot connect to the server $!\n";
        $ihlt->kill_kill;
        die;
    }
    pass "$_: connected to the server";
}

my $response = "";
sleep 3;

$sockets[2]->autoflush(1);
is $sockets[2]->send("wall Hello World!"), 17, 'wall msg began';

# data to send to a server
$sockets[1]->autoflush(1);
is $sockets[1]->send("wall Hello World!\n"), 18, 'wall msg sent';
$sockets[2]->recv( $response, 28 );
is $response, "211 wall: Hello World!\r\n", 'wall msg recv';
$sockets[3]->recv( $response, 28 );
is $response, "211 wall: Hello World!\r\n", 'wall msg recv again';

$sockets[3]->autoflush(1);
is $sockets[3]->send("next Hello World!\n"), 18, 'next msg sent';
$sockets[1]->recv( $response, 28 );
is $response, "211 prev: Hello World!\r\n", 'next msg recv';

is $sockets[1]->send("prev Hello World!\n"), 18, 'prev msg sent';
$sockets[3]->recv( $response, 28 );
is $response, "211 next: Hello World!\r\n", 'prev msg recv';

is $sockets[3]->send("ping Hello World!\n"), 18, 'echo msg sent';
$sockets[3]->recv( $response, 28 );
is $response, "211 pong: Hello World!\r\n", 'echo msg recv';

is $sockets[1]->send("bad Hello World!\n"), 17, 'bad msg sent';
$sockets[1]->recv( $response, 53 );
is $response, "502 Bad command or it is not implemented here.\r\n",
  'bad msg recv';

$sockets[1]->blocking(0);
$sockets[1]->recv( $response, 28 );
$sockets[1]->blocking(1);
cmp_ok $!, '==', $!{EWOULDBLOCK}, 'recv would block';
is $response, "", 'recv string empty';
$sockets[3]->blocking(0);
$sockets[3]->recv( $response, 28 );
$sockets[3]->blocking(1);
cmp_ok $!, '==', $!{EWOULDBLOCK}, 'recv would block again';
is $response, "", 'recv string empty again';

is $sockets[2]->send("\n"), 1, 'wall msg sent';
$sockets[1]->recv( $response, 28 );
is $response, "211 wall: Hello World!\r\n", 'wall msg recv';
$sockets[3]->recv( $response, 28 );
is $response, "211 wall: Hello World!\r\n", 'wall msg recv';

is $sockets[2]->send("quit\r\n"), 6, 'sent quit';

ok $sockets[$_]->close(), "$_: closed" for ( 1 .. 3 );

$ihlt->finish();
is $ihlt->result(0), 0, 'valgrind ok';

exit 0;
