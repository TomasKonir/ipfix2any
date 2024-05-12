#include "filter_zero.h"
#include <QJsonArray>

FilterZero::FilterZero(const QJsonObject params) : Filter(params) {
    qInfo() << "Filter zero started";
}

bool FilterZero::next(output_row_t& row) {
    for (int i = 0; i < row.fields.count(); i++) {
        if (row.fields[i].value == "0") {
            row.fields.removeAt(i);
            i--;
        }
    }
    return (true);
}
