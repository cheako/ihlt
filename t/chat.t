use Test::More tests => 21;
use IPC::Run qw(start);

my $ihlt =
  start( [ 'valgrind', '--error-exitcode=63', 'src/ihlt' ], undef, '>&2' );
END { $ihlt->kill_kill; }

sleep 3;
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
is $sockets[2]->send("Hello World"), 11, 'third msg began';

# data to send to a server
$sockets[1]->autoflush(1);
is $sockets[1]->send("Hello World\n"), 12, 'first msg sent';
$sockets[2]->recv( $response, 20 );
is $response, "Hello,World!\n", 'first msg recv';
$sockets[3]->recv( $response, 20 );
is $response, "Hello,World!\n", 'first msg recv again';

$sockets[3]->autoflush(1);
is $sockets[3]->send("Hello World\n"), 12, 'second msg sent';
$sockets[2]->recv( $response, 20 );
is $response, "Hello,World!\n", 'second msg recv';
$sockets[1]->recv( $response, 20 );
is $response, "Hello,World!\n", 'second msg recv again';

$sockets[1]->blocking(0);
$sockets[1]->recv( $response, 20 );
$sockets[1]->blocking(1);
cmp_ok $!, '==', $!{EWOULDBLOCK}, 'recv would block';
is $response, "", 'recv string empty';
$sockets[3]->blocking(0);
$sockets[3]->recv( $response, 20 );
$sockets[3]->blocking(1);
cmp_ok $!, '==', $!{EWOULDBLOCK}, 'recv would block again';
is $response, "", 'recv string empty again';

is $sockets[2]->send("\n"), 1, 'third msg sent';
$sockets[1]->recv( $response, 20 );
is $response, "Hello,World!\n", 'third msg recv';
$sockets[3]->recv( $response, 20 );
is $response, "Hello,World!\n", 'third msg recv again';

ok $sockets[$_]->close(), "$_: closed" for ( 1 .. 3 );

$ihlt->kill_kill;
END { }
$ihlt->finish();
is $ihlt->result(0), 0, 'valgrind ok';

exit 0;
