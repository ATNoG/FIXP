# Future Internet Fusion (FIFu)
Proof-of-concept developed under my PhD to enable interoperation between
IP and ICN (NDN and PURSUIT) networks.

FIFu is composed by two main components:
 - **FIXP:** compose the adaptation layer, converting messages from/to
             different network protocols;
 - **FIC (not implemented):** acting as the intelligent layer responsible for configuring,
            managing and supporting the FIXP operation, as well as for storing,
            managing and maintaining the identifiers of resources on each network architectures.

## FIXP 
Modules:
 - **Core:** Main logic of the FIXP, acting as a broker between plugins
 - **Protocol Plugins:** Support for new L3/L4/L5 protocols 
   - _HTTP_
   - _NDN_
   - _PURSUIT_
   - _PURSUIT (multipath)_
 - **Converter Plugins:** Support for content conversion between different networks protocols
   - _HTML_: converts links towards the destination network protocol

## Tools
 - ficatresource (Future Internet Cat Resource): Get a resource matching a
   given URI and write it to stdout.
   
## Citation
If you use this code in any way, please cite the following work:

Guimarães, C., Quevedo, J., Ferreira, R., Corujo, D. and Aguiar, R.L., 2019.
Exploring interoperability assessment for Future Internet Architectures roll out.
Journal of Network and Computer Applications, 136, pp.38-56.

https://doi.org/10.1016/j.jnca.2019.04.008.

## License
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
