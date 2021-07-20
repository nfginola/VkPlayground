#include "pch.h"
#include "AssimpLoader.h"

//#include <assimp/Importer.hpp>      // C++ importer interface
//#include <assimp/scene.h>           // Output data structure
//#include <assimp/postprocess.h>     // Post processing flags

namespace Nagi
{

	AssimpLoader::AssimpLoader(const std::filesystem::path& filePath)
	{
		Assimp::Importer importer;


		const aiScene* scene = importer.ReadFile(
			filePath.relative_path().string().c_str(),
			aiProcess_Triangulate |
			aiProcess_FlipUVs |			// Vulkan screen space is 0,0 on top left
			aiProcess_GenNormals
		);

		if (scene == nullptr)
			throw std::runtime_error(std::string("Assimp: File not found! : ") + filePath.filename().string());

		// Pre-allocate memory for resources
		unsigned int totalVertexCount = 0;
		unsigned int totalSubsetCount = 0;
		for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
			totalVertexCount += scene->mMeshes[i]->mNumVertices;

		m_vertices.reserve(totalVertexCount);
		m_indices.reserve(totalVertexCount);
		m_subsets.reserve(scene->mNumMeshes);

		// Grab materials
		m_materials.reserve(scene->mNumMaterials);
		for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
		{
			auto mtl = scene->mMaterials[i];
			aiString diffPath, norPath, opacityPath, specularPath;
			mtl->GetTexture(aiTextureType_DIFFUSE, 0, &diffPath);
			mtl->GetTexture(aiTextureType_HEIGHT, 0, &norPath);
			mtl->GetTexture(aiTextureType_OPACITY, 0, &opacityPath);
			mtl->GetTexture(aiTextureType_SPECULAR, 0, &specularPath);

			AssimpMaterialPaths matPaths;
			matPaths.diffuseFilePath = (diffPath.length == 0) ? std::nullopt : std::optional<std::string>(diffPath.C_Str());
			matPaths.normalFilePath = (norPath.length == 0) ? std::nullopt : std::optional<std::string>(norPath.C_Str());
			matPaths.opacityFilePath = (opacityPath.length == 0) ? std::nullopt : std::optional<std::string>(opacityPath.C_Str());
			matPaths.specularFilePath = (specularPath.length == 0) ? std::nullopt : std::optional<std::string>(specularPath.C_Str());
			m_materials.push_back(matPaths);
		}


		// Start processing
		processNode(scene->mRootNode, scene);
	}

	const std::vector<AssimpVertex>& AssimpLoader::getVertices() const
	{
		return m_vertices;
	}

	const std::vector<uint32_t>& AssimpLoader::getIndices() const
	{
		return m_indices;
	}

	const std::vector<AssimpMeshSubset>& AssimpLoader::getSubsets() const
	{
		return m_subsets;
	}

	const std::vector<AssimpMaterialPaths> AssimpLoader::getMaterials() const
	{
		return m_materials;
	}

	void AssimpLoader::processMesh(aiMesh* mesh, const aiScene* scene)
	{
		for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
		{
			AssimpVertex vert{};
			vert.position = mesh->mVertices[i];
			//vert.position[0] = mesh->mVertices[i].x;
			//vert.position[1] = mesh->mVertices[i].y;
			//vert.position[2] = mesh->mVertices[i].z;

			vert.normal = mesh->mNormals[i];
			//vert.normal[0] = mesh->mNormals[i].x;
			//vert.normal[1] = mesh->mNormals[i].y;
			//vert.normal[2] = mesh->mNormals[i].z;

			if (mesh->mTextureCoords[0])
			{
				vert.uv.x = mesh->mTextureCoords[0][i].x;
				vert.uv.y = mesh->mTextureCoords[0][i].y;
			}

			m_vertices.push_back(vert);
		}

		unsigned int indicesThisMesh = 0;
		for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
		{
			aiFace face = mesh->mFaces[i];

			for (unsigned int j = 0; j < face.mNumIndices; ++j)
			{
				m_indices.push_back(face.mIndices[j]);
				++indicesThisMesh;
			}

		}

		// Get material
		auto mtl = scene->mMaterials[mesh->mMaterialIndex];
		aiString diffPath, norPath, opacityPath;
		mtl->GetTexture(aiTextureType_DIFFUSE, 0, &diffPath);
		mtl->GetTexture(aiTextureType_HEIGHT, 0, &norPath);
		mtl->GetTexture(aiTextureType_OPACITY, 0, &opacityPath);

		// Subset data
		AssimpMeshSubset subsetData = { };
		subsetData.diffuseFilePath = (diffPath.length == 0) ? std::nullopt : std::optional<std::string>(diffPath.C_Str());
		subsetData.normalFilePath = (norPath.length == 0) ? std::nullopt : std::optional<std::string>(norPath.C_Str());
		subsetData.opacityFilePath = (opacityPath.length == 0) ? std::nullopt : std::optional<std::string>(opacityPath.C_Str());

		subsetData.vertexStart = m_meshVertexCount;
		m_meshVertexCount += mesh->mNumVertices;

		subsetData.indexCount = indicesThisMesh;
		subsetData.indexStart = m_meshIndexCount;
		m_meshIndexCount += indicesThisMesh;

		m_subsets.push_back(subsetData);
	}

	void AssimpLoader::processNode(aiNode* node, const aiScene* scene)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			processMesh(mesh, scene);
		}

		for (unsigned int i = 0; i < node->mNumChildren; ++i)
			processNode(node->mChildren[i], scene);
	}

}
