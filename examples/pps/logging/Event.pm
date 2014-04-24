package Event;
use base qw(Happening Described);


########################################################################


sub new ($$$) {
    my ($proto, $proc, $desc, $time) = @_;
    
    my $self = $proto->SUPER::new($proc);
    $self->{'desc'} = $desc;
    $self->{'time'} = $time;
    
    $self;
}


########################################################################


sub time ($) {
    return shift->{'time'};
}


########################################################################


sub params ($) {
    my $self = shift;
    return ($self->psDesc, $self->proc,
	    $self->time, 'event');
}


########################################################################


1;
