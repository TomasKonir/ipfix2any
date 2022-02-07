#include <QJsonArray>
#include "filter_fields.h"

FilterFields::FilterFields(const QJsonObject params) : Filter(params){
	QJsonArray pass_p = params.value("pass").toArray();
	for(const auto &v : pass_p){
		if(v.isString()){
			pass << v.toString();
		}
	}
	QJsonArray remove_p = params.value("remove").toArray();
	for(const auto &v : remove_p){
		if(v.isString()){
			remove << v.toString();
		}
	}
}

bool FilterFields::next(output_row_t &row){
	for(int i=0;i<row.fields.count();i++){
		if(pass.count() && !pass.contains(row.fields[i].name)){
			row.fields.removeAt(i);
			i--;
		} else if(remove.count() && remove.contains(row.fields[i].name)){
			row.fields.removeAt(i);
			i--;
		}
	}
	return(true);
}
