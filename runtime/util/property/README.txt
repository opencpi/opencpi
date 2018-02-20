This file is protected by Copyright. Please refer to the COPYRIGHT file
distributed with this source distribution.

This file is part of OpenCPI <http://www.opencpi.org>

OpenCPI is free software: you can redistribute it and/or modify it under the
terms of the GNU Lesser General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program. If not, see <http://www.gnu.org/licenses/>.

This property facility manages runtime properties, types, and values.

Properties are named and typed and are associated with some other "thing" that wants to have readable and writable properties.

Property value lists, and lists of name/typed-value pairs that are used to convey a set of named values as parameter to various APIs, and can also be used to specify a set of values to be set on an object that has properties.

Typed values are essentially a union type that can hold any of the allowable typed values.

The set of potential types of value are limited to be a sweet spot of simplicity and convenience, and are intended to at least encompass property types as defined in the SCA specification.
