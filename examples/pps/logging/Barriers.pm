package Barriers;
use base qw(Spans);

use Barrier;

require Exporter;
@EXPORT = qw();


########################################################################


sub merge ($$$) {
    my ($self, $begin, $end) = @_;
    return new Barrier $begin, $end;
}


########################################################################


sub things ($$) {
    return 'barriers';
}


########################################################################


1;
