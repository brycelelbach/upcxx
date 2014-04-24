package Barrier;
use base qw(Span);

require Exporter;
@EXPORT = qw();


########################################################################


sub params ($) {
    my $self = shift;
    return ($self->proc,
	    $self->begin, $self->end,
	    'barrier');
}


########################################################################


1;
