/*
 * Material.cpp
 *
 *  Created on: 16 feb 2012
 *      Author: johan
 */

#include <model/Material.h>
#include <model/Base.h>
#include <model/Strand.h>

#include <maya/MFileIO.h>
#include <maya/MGlobal.h>
#include <maya/MCommandResult.h>

#include <DNA.h>

#include <algorithm>
#include <iterator>

/*
 * New code for managing materials, using this code, duplication of material nodes should never occur
 * We're also no longer dependent on the DNAshaders.ma file, which makes it so much easier for users to install the plugin
 *
 */

#define MEL_SETUP_MATERIALS_COMMAND																							\
	"$materials = `ls -mat \"DNA*\"`;\n"																					\
	"string $material_sets[];\n"																							\
	"clear($material_sets);\n"																								\
																															\
	"if (size($materials) == 0) {\n"																						\
	"	matrix $colors[10][3] = <<\n"																						\
	"		0, 1, 0.45666122 ;\n"																							\
	"		1, 0.18333334, 0 ;\n"																							\
	"		1, 1, 1 ;\n"																									\
	"		0.75, 0.75, 0.75 ;\n"																							\
	"		0, 0, 1 ;\n"																									\
	"		1, 1, 0 ;\n"																									\
	"		1, 0, 1 ;\n"																									\
	"		0.20863661, 0.20863661, 0.20863661 ;\n"																			\
	"		0, 1, 1 ;\n"																									\
	"		1, 0.12300003, 0.29839987\n"																					\
	"	>>;\n"																												\
																															\
	"	for($i = 0; $i < 10; ++$i) {\n"																						\
	"		$node = `createNode lambert -ss -n (\"DNA\" + $i)`;\n"															\
	"		setAttr ($node + \".dc\") 1;\n"																					\
	"		setAttr ($node + \".c\") -type \"float3\" $colors[$i][0] $colors[$i][1] $colors[$i][2];\n"						\
	"		connectAttr ($node + \".msg\") \":defaultShaderList1.s\" -na;\n"												\
																															\
	"		$materials[size($materials)] = $node;\n"																		\
	"	}\n"																												\
	"}\n"																													\
																															\
	"for ($material in $materials) {\n"																						\
	"	$sets = `listSets -as -t 1`;\n"																						\
																															\
	"	$exists = false;\n"																									\
																															\
	"	for ($set in $sets) {\n"																							\
	"		if ($set == \"SurfaceShader_\" + $material) {\n"																\
	"			$exists = true;\n"																							\
	"			break;\n"																									\
	"		}\n"																											\
	"	}\n"																												\
																															\
	"	string $current_set;\n"																								\
																															\
	"	if (!$exists) {\n"																									\
																															\
	"		$current_set = `sets -renderable true -noSurfaceShader true -empty -name (\"SurfaceShader_\" + $material)`;\n"	\
	"		connectAttr ($material + \".outColor\") ($current_set + \".surfaceShader\");\n"									\
	"	}\n"																												\
	"	else\n"																												\
	"		$current_set = \"SurfaceShader_\" + $material;\n"																\
																															\
	"	$material_sets[size($material_sets)] = $current_set;\n"																\
	"}\n"																													\
																															\
	"ls $material_sets;\n"

/*
 * Notice that this code is also defined in the old DNA.cpp. So in case it changes before this file becomes the main version..
 */

/*#define MEL_PARSE_MATERIALS_COMMAND			\
	"$materials = `ls -mat \"DNA*\"`;\n"	\
	"string $colors[];\n"	\
	"for ($material in $materials) {\n"	\
    "$dnaShader = `sets -renderable true -noSurfaceShader true -empty -name (\"SurfaceShader_\" + $material)`;\n"	\
    "connectAttr ($material + \".outColor\") ($dnaShader + \".surfaceShader\");\n"	\
    "$colors[size($colors)] = $dnaShader;\n"	\
	"}\n"	\
	"ls $colors;\n"

#define MEL_MATERIALS_EXIST_COMMAND			\
	"ls -mat \"DNA*\";"
*/

namespace Helix {
	namespace Model {
		std::vector<Material> Material::s_materials;
		//std::vector<std::pair<MString, bool> > Material::s_materialFiles;

		class PaintStrandWithMaterialFunctor {
		public:
			inline PaintStrandWithMaterialFunctor(Material & m, MString *_tokenize) : material(&m), tokenize(_tokenize) {

			}

			MStatus operator()(Base & base) {
				MStatus status;
				MDagPath base_dagPath = base.getDagPath(status);

				if (!status) {
					status.perror("Base::getDagPath");
					return status;
				}

				(*tokenize) += base_dagPath.fullPathName(&status) + " ";

				return status;
			}

		private:
			Material *material;
			MString *tokenize;
		};

		Material::Iterator Material::AllMaterials_begin(MStatus & status, Material::Container::size_type & numMaterials) {
			Iterator begin = AllMaterials_begin(status);
			numMaterials = s_materials.size();
			return begin;
		}
		
		Material::Iterator Material::AllMaterials_begin(MStatus & status) {
			MCommandResult commandResult;
			MStringArray materialNames;

			s_materials.clear();

			/*
			 * New code: All we need to do is execute the above command. The result will be a string array of the materials available
			 */

			if (!(status = MGlobal::executeCommand(MEL_SETUP_MATERIALS_COMMAND, commandResult))) {
				status.perror("MGlobal::executeCommand");
				return AllMaterials_end();
			}

			if (!(status = commandResult.getResult(materialNames))) {
				status.perror("MCommandResult::getResult");
				return AllMaterials_end();
			}

			s_materials.reserve(materialNames.length());
			std::copy(&materialNames[0], &materialNames[0] + materialNames.length(), std::back_insert_iterator<Container>(s_materials));

			return &s_materials[0];
		}

		/*MStatus Material::GetAllMaterials(Material **materials, size_t & numMaterials) {
			

			/*
			 * Make sure the materials exist
			 */

			/*{
				if (!(status = MGlobal::executeCommand(MEL_MATERIALS_EXIST_COMMAND, commandResult))) {
					status.perror("MGlobal::executeCommand 1");
					return status;
				}

				MStringArray result;

				if (!(status = commandResult.getResult(result))) {
					status.perror("MCommandResult::getResult");
					return status;
				}

				if (result.length() == 0) {
					s_materials.clear();
					ResetMaterialFilesLoaded();
				}
			}

			while(s_materials.size() == 0) {
				CacheMaterials();

				/*
				 * Find the materials, if we can't find any, let the loop spin
				 *

				if (!(status = MGlobal::executeCommand(MEL_PARSE_MATERIALS_COMMAND, commandResult))) {
					status.perror("MGlobal::executeCommand");
					return status;
				}

				if (!(status = commandResult.getResult(materialNames))) {
					status.perror("MCommandResult::getResult");
					return status;
				}

				s_materials.clear();
				s_materials.reserve(materialNames.length());

				for(unsigned int i = 0; i < materialNames.length(); ++i)
					s_materials.push_back(Material(materialNames[i]));
			}

			*materials = &s_materials[0];
			numMaterials = s_materials.size();

			return MStatus::kSuccess;
		}*/

		/*
		 * Helper method
		 */

		/*MStatus UserImportFile(MString title) {
			MStatus status;
			MCommandResult commandResult;
			if (!(status = MGlobal::executeCommand(MString("fileDialog2 -fileFilter \"Maya Files (*.ma *.mb)\" -dialogStyle 2 -caption \"" + title + "\" -fileMode 1;"), commandResult))) {
				status.perror("MGlobal::executeCommand");
				return status;
			}

			MStringArray stringArray;
			if (!(status = commandResult.getResult(stringArray))) {
				status.perror("MCommandResult::getResult");
				return status;
			}

			for(unsigned int i = 0; i < stringArray.length(); ++i) {
				if (!(status = MFileIO::importFile(stringArray[i])))
					status.perror(MString("Failed to open the file \"") + stringArray[i] + "\"");
			}

			return MStatus::kSuccess;
		}

		MStatus Material::CacheMaterials() {
			MStatus status;

			bool empty = true;

			for(std::vector<std::pair<MString, bool> >::iterator it = s_materialFiles.begin(); it != s_materialFiles.end(); ++it) {
				if (!it->second) {
					empty = false;

					if (!(status = MFileIO::importFile(it->first))) {
						status.perror(MString("Failed to open the file \"") + it->first + "\". Asking the user for the file to open");

						if (!(status = UserImportFile(MString("Can't locate the file \"") + it->first + "\""))) {
							status.perror("Failed to open user requested file");
							return status;
						}
					}

					it->second = true;
				}
			}

			if (empty) {
				if (!(status = UserImportFile("No material files could be found. Please select a file containing suitable materials")))
					return MStatus::kFailure;
			}

			return MStatus::kSuccess;
		}

		void Material::ResetMaterialFilesLoaded() {
			for(std::vector<std::pair<MString, bool> >::iterator it = s_materialFiles.begin(); it != s_materialFiles.end(); ++it) {
				it->second = false;
			}
		}

		Material::RegisterMaterialFile defaultMaterialFile(DNASHADERS_SOURCE_FILE);*/
	}
}
