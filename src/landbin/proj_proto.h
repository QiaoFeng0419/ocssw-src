double asinz(double con);
double adjust_lon(double x);
int robforint(double r, double center_long, double false_east, double false_north);
int robinvint(double r, double center_long, double false_east, double false_north);
int robfor(double lon, double lat, double *x, double *y);
int robinv(double x, double y, double *lon, double *lat);
int molwforint(double r, double center_long, double false_east, double false_north);
int molwinvint(double r, double center_long, double false_east, double false_north);
int molwfor(double lon, double lat, double *x, double *y);
int molwinv(double x, double y, double *lon, double *lat);
int hamforint(double r, double center_long, double false_east, double false_north);
int haminvint(double r, double center_long, double false_east, double false_north);
int hamfor(double lon, double lat, double *x, double *y);
int haminv(double x, double y, double *lon, double *lat);
