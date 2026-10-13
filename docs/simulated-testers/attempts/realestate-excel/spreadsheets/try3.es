read "listings.xlsx" and call it Data
For each Row in Data:
    say Row Neighborhood
    say AVERAGEIF of Data Price where Neighborhood is Row Neighborhood
    say MINIMUM of Data Price where Neighborhood is Row Neighborhood
