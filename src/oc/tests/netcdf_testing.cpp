
#include <iostream>

#include <netcdf>

using namespace std;

using namespace netCDF;
using namespace netCDF::exceptions;

static const string test_file = "/accounts/jlefler/workspace/oc/testdata/l2gen/aqua/A2008080214500.L2_LAC_IOP.subline.nc";

int main()
{
    cout << "NetCDF testing\n";

    try {
        NcFile data_file(test_file, NcFile::read);

        auto nav_data = data_file.getGroup("navigation_data");
        // Get the latitude and longitude variables and read data.
        auto latVar = nav_data.getVar("latitude");
        if (latVar.isNull()){
            cout << "latVar = null\n";
            return 0;
        }
        const auto dims = latVar.getDims();
        cout << "dims: ";
        for (auto d : dims){
            cout << d.getName() << " (" << d.getSize() << ") ";
        }
        cout << "\n";
        std::vector<std::vector<double>> lines(5);
        std::vector<double> pixels(dims[1].getSize());
        for (size_t i=0;i<110;i++){
            latVar.getVar(vector<size_t>{i, 0}, vector<size_t>{1, pixels.capacity()}, pixels.data());
            // latVar.getVar(vector<size_t>{i, 0}, pixels.data());

            cout << "data row: ";
            for (const auto d : pixels){
                cout << d << " ";
            }
            cout << "\n";
        }

        // latVar.get
    } catch (NcException& e) {
        cout << "FAILURE**************************\n";
        cout << e.what() << "\n";
        cout << e.errorCode() << "\n";
        return 0;
    }
    cout << "Done NetCDF Testing\n";
    return 0;
}
