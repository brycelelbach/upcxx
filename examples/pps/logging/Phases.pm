package Phases;
use base qw(Spans);

use Phase;

require Exporter;
@EXPORT = qw();


########################################################################


sub new ($) {
    my $self = shift->SUPER::new;
    $self->{maxDepth} = 0;
    return $self;
}


########################################################################


sub merge ($$$) {
    my ($self, $begin, $end) = @_;
    
    my $depth = 1 + @{$self->pending};
    my $max = $self->maxDepth;
    $self->{maxDepth} = $depth if $depth > $max;
    
    return new Phase $begin, $end, $depth;
}


########################################################################


sub maxDepth ($) {
    return shift->{maxDepth};
}


########################################################################


sub things ($$) {
    return 'phases';
}


sub print ($$) {
    my ($self, $out) = @_;
    
    $out->print('/maxDepth ', $self->maxDepth, " def\n");    
    $self->SUPER::print($out);
}


########################################################################


1;
