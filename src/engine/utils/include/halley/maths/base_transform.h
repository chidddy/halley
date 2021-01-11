/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include "polygon.h"
#include "vector2.h"

namespace Halley {
	class Base2D {
	public:
		Base2D();
		Base2D(Vector2f u, Vector2f v);
		explicit Base2D(const ConfigNode& node);

		Vector2f transform(Vector2f point) const;
		Vector2f inverseTransform(Vector2f point) const;

		Polygon transform(const Polygon& poly) const;
		Polygon inverseTransform(const Polygon& poly) const;

		static Vector2f transform(Vector2f point, Vector2f u, Vector2f v);

		Base2D getInverse() const;

		ConfigNode toConfigNode() const;

	private:
		Vector2f u, v;
		Vector2f invU, invV;

		Base2D(Vector2f u, Vector2f v, Vector2f invU, Vector2f invV);
	};
}