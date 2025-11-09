# EconomicVisualizer
Visualization of US population economic data, used for showing areas of economic distress.

TO USE:
Running main will load the data into structures. The data set is over 300,000
data point, so this may take a moment. After loading, you can take a look at the map or
hover over states to see the ones that might be the most threatened economically based on
the weighting of different attributes. Using the search function on the left, you can get specific
data points by putting in a state, county, year (2001-2023) and an attribute. For any attributes
that don't change on a year by year basis, use 2023.

BONUS: Line 144 of the tree.cpp file contains our magic weights that create the coloring on
our map. These are used to weight certain attributes more than others. These can be
changed to alter the coloring on our map, highlighting in darker red the areas most at risk
based on the weighting of your statistics.


    NOTE: Data is currently organized by county. More attributes can be loaded but 
    would require updates to data structures and file parsing.