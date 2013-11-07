#pragma once

#include <string>
#include <vector>

const std::string Picasa_ListURL = "http://picasaweb.google.com/data/feed/api/featured/?kind=photo&imgmax=1600&max-results=15";

std::string Picasa_GetDirectImageURL( const std::string& BaseURL, int ImgSizeType );
void Picasa_ParseXMLResponse( const std::string& Response, std::vector<std::string>& URLs );
