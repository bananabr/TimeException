//
//  TRIMMEAN.h
//  TRIMMEAN
//
//  Created by Kevin Tan on 6/11/17.
//

#ifndef trimmean
#define trimmean

enum ErrorNumber {

    EBADN,
    EBADPCNT,
    EBADARR,
    MALLOC_FAILED

};

double slowTRIMMEAN(long long int inputArray[], long long int n, double percent, ErrorNumber* errorno = nullptr);
double TRIMMEAN(long long int inputArray[], long long int n, double percent, ErrorNumber* errorno = nullptr);

#endif /* trimmean defined */