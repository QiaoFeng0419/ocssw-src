
FileReader = reads input files
DataProvider = provides data during processing (algorithm, LUT, ancillary, etc)

A DataProvider can also be a FileReader; ETOPO1 class can provide water_depth/etc, but can also read input ETOPO files
    Due to this, there should probably be a single directory for modules, otherwise the same code will need to be compiled into the reader directory and algorithm directory
    Every module will need to implement both DataProviderProvider and FileReaderProvider, then?  What if we add more entry points, all modules will break?
        Can I just catch exceptions when trying to find classes that don't exist?

DataRecord...

It was simple, but the record needs support for products with arbitrary dimensions (wavelength, angle, etc).
    Can I pull this off with a template?
    Do other products that use them have to know about them?  What if angle or some other weird dimension is irrelevant, or something?
    When products Require others, how should they add constraints (such as "I only need X, Y, Z wavelengths")?

Who is in charge of attributes?  Products?  Or DataProviders?
...Who defines products?  Central product database, like now?  What about third parties?

The "smartest" or most obvious thing to do is to emulate NetCDF/HDF concepts.  It'd be flexible, but cumbersome for new modules.
    Any information that less expressive data formats can't represent is wasted processing, but that's probably fine.
    (This is also more cumbersome to start.)

Epiphany?  For N dimensional, geospatial variables, make them merely "parents" of variables.  If something says it needs wavelength-412, have it create a "normal" geospatial variable for it to point to.

What to do with non-geospatial variables?  Do geospatial variables need static dimensions or can lat/lon be in any order?  What about date dimensions?

Should variables just have a "get(lat, lon)" or "get(line_i, pixel_i)" or whatever to get single points?  What about getting boxes?  Quick functions to get min/mean/stddev/etc?
