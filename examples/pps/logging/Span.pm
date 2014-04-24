package Span;
use base qw(Happening);

require Exporter;
@EXPORT = ();


########################################################################


sub new ($$$) {
    my ($proto, $begin, $end) = @_;
    $proto->same($begin, $end, 'proc');
    
    my $self = $proto->SUPER::new($begin->proc);
    $self->{begin} = $begin->time;
    $self->{end} = $end->time;

    $self;
}


sub same ($$$) {
    my ($self, $begin, $end, $field) = @_;

    my $beginValue = $begin->{$field};
    my $endValue = $end->{$field};
    
    $beginValue eq $endValue
	or die "$field mismatch";
}


########################################################################


sub proc ($) {
    return shift->{proc};
}


sub begin ($) {
    return shift->{begin};
}


sub end ($) {
    return shift->{end};
}


1;
