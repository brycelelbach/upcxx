package Phase;
use base qw(Span Described);

require Exporter;
@EXPORT = qw();


########################################################################


sub new ($$$) {
    my ($proto, $begin, $end, $depth) = @_;
    $proto->same($begin, $end, 'desc');
    
    my $self = $proto->SUPER::new($begin, $end);
    $self->{depth} = $depth;
    $self->{desc} = $begin->desc;

    $self;
}


########################################################################


sub depth ($) {
    return shift->{depth};
}


########################################################################


sub params ($) {
    my $self = shift;
    return ($self->psDesc,
	    $self->proc,
	    $self->begin, $self->end,
	    $self->depth,
	    'phase');
}


########################################################################


1;
