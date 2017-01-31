use Test::More tests => 49;
use IPC::Run qw(start);

my $ihlt =
  start( [ 'valgrind', '--error-exitcode=63', 'src/ihlt', '-f' ], undef,
    '>&2' );

use IO::Socket::INET;

# create a connecting socket
my @sockets;
for ( 1 .. 3 ) {
    my $ctr = 0;
    do {
        diag "Connection $_ try $ctr: $!"
          if ( $ctr != 0 );
        sleep( ( $_ == 1 && $ctr == 0 ) * 20 + $ctr );
        $sockets[$_] = new IO::Socket::INET(
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

is $sockets[1]->send("listen 8AAAA\n"), 13, 'listen 0xF0 sent';
$sockets[1]->recv( $response, 128 );
warn $response;
is $sockets[3]->send("send 8AAAA test12\n"), 18, 'send msg to 0xF0';
$sockets[1]->recv( $response, 28 );
is $response, "211 send: 8AAAA test12\r\n", 'channel 0xF0 msg recv';

is $sockets[2]->send("listen 8AAADw\n"), 14, 'listen 0xF000000F sent';
$sockets[2]->recv( $response, 128 );
warn $response;
is $sockets[1]->send("send 8AAADw test1\n"), 18, 'send msg to 0xF000000F';
$sockets[2]->recv( $response, 28 );
is $response, "211 send: 8AAADw test1\r\n", 'channel 0xF000000F msg recv';

# 1483250400  Sun Jan  1 00:00:00 CST 2017
# 1485962931 - 1483250400 = 2712531
is $sockets[1]->send("store ok 3 2712531 ::1 4458 dat hi\n"), 35, 'store long msg sent';
$sockets[1]->recv( $response, 4096 );
is $response, "504 Too many parameters.\r\n", 'store ok msg recv';

is $sockets[1]->send("store ok 3 2712531 ::1\n"), 23, 'store short msg sent';
$sockets[1]->recv( $response, 4096 );
is $response, "501 Too few parameters.\r\n", 'store ok msg recv';

is $sockets[1]->send("store ok hi 2712531 ::1 4458\n"), 29, 'store rating msg sent';
$sockets[1]->recv( $response, 4096 );
is $response, "501 Invalid rating.\r\n", 'store rating msg recv';

is $sockets[1]->send("store ok 3 hi ::1 4458\n"), 23, 'store time msg sent';
$sockets[1]->recv( $response, 4096 );
is $response, "501 Invalid time.\r\n", 'store time msg recv';

is $sockets[1]->send("store ok 3 2712531 hi 4458\n"), 27, 'store address msg sent';
$sockets[1]->recv( $response, 4096 );
is $response, "501 Invalid network address.\r\n", 'store address msg recv';

is $sockets[1]->send("store ok 3 2712531 ::1 hi\n"), 26, 'store port msg sent';
$sockets[1]->recv( $response, 4096 );
is $response, "501 Invalid port.\r\n", 'store port msg recv';

is $sockets[1]->send("store ok 3 2712531 ::1 4458 word\n"), 33, 'store good msg sent';
$sockets[1]->recv( $response, 4096 );
is $response, "220 Saved word\r\n", 'store good msg recv';

is $sockets[1]->send("load ok\n"), 8, 'load msg sent';
$sockets[1]->recv( $response, 4096 );
is $response, "220 ok is 3 2712531 ::1 4458 word\r\n", 'load msg recv';

is $sockets[2]->send("quit\r\n"), 6, 'sent quit';

ok $sockets[$_]->close(), "$_: closed" for ( 1 .. 3 );

$ihlt->finish();
is $ihlt->result(0), 0, 'valgrind ok';

exit 0;
