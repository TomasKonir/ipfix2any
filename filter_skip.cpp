#include "filter_skip.h"
#include <QJsonArray>

FilterSkip::FilterSkip(const QJsonObject params) : Filter(params) {
    QJsonArray skip_p = params.value("skip").toArray();
    for (const auto& v : skip_p) {
        if (v.isString()) {
            skip << v.toString();
        }
    }
    qInfo() << skip;
}

bool FilterSkip::next(output_row_t& row) {
    if (skip.count() == 0) {
        return (true);
    }
    for (const output_field_t &f : row.fields) {
        if (skip.contains(f.name)) {
            return(false);
        }
    }
    return (true);
}
