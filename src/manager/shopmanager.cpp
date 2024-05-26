#include "shopmanager.h"
#include "packetmanager.h"
#include "usermanager.h"
#include "itemmanager.h"
#include "nlohmann/json.hpp"
#include "keyvalues.hpp"

using namespace std;
using ordered_json = nlohmann::ordered_json;

#define SHOP_JSON_VERSION 1

CShopManager g_ShopManager;

CShopManager::CShopManager() : CBaseManager("ShopManager")
{
}

CShopManager::~CShopManager()
{
}

bool CShopManager::Init()
{
	KVToJson();

	if (!LoadProducts())
		return false;

	Logger().Info("[Shop] Loaded %d products.\n", m_Products.size());

	return true;
}

void CShopManager::Shutdown()
{
	m_Products.clear();
}

bool CShopManager::LoadProducts()
{
	try
	{
		ifstream f("Shop.json");
		ordered_json cfg = ordered_json::parse(f, nullptr, false, true);

		if (cfg.is_discarded())
		{
			Logger().Warn("CShopManager::LoadProducts: couldn't load Shop.json.\n");
			return true; // just a warning
		}

		int version = cfg.value("Version", 0);
		if (version != SHOP_JSON_VERSION)
		{
			Logger().Error("CShopManager::LoadProducts: %d != SHOP_JSON_VERSION(%d)\n", version, SHOP_JSON_VERSION);
			return false;
		}

		if (cfg.contains("Recommended"))
		{
			ordered_json jRecommended = cfg["Recommended"];
			for (auto& page : jRecommended)
			{
				m_RecommendedProducts.push_back(page.get<vector<int>>());
			}
		}

		m_PopularProducts = cfg.value("Popular", m_PopularProducts);

		int productTypeId = 1;
		if (cfg.contains("Products"))
		{
			ordered_json jProducts = cfg["Products"];
			for (auto& jProduct : jProducts.items())
			{
				Product product;
				product.relationProductID = stoi(jProduct.key());
				product.isPoints = jProduct.value().value("IsPoints", 0);

				if (jProduct.value().contains("SubProducts"))
				{
					ordered_json jSubProducts = jProduct.value()["SubProducts"];
					for (auto& iSubProduct : jSubProducts.items())
					{
						SubProduct subProduct;
						ordered_json jSubProduct = iSubProduct.value();

						if (jSubProduct.contains("Items"))
						{
							ordered_json jItems = jSubProduct["Items"];
							for (auto& iItems : jItems.items())
							{
								RewardItem item;
								item.itemID = stoi(iItems.key());
								item.duration = iItems.value().value("Duration", 0);
								item.count = iItems.value().value("Count", 1);
								subProduct.items.push_back(item);
							}
						}
						else
						{
							RewardItem item;
							item.itemID = product.relationProductID;
							item.duration = jSubProduct.value("Duration", 0);
							item.count = jSubProduct.value("Count", 1);
							subProduct.items.push_back(item);
						}

						subProduct.productID = productTypeId++;
						subProduct.price = jSubProduct.value("Price", 0);
						subProduct.additionalPoints = jSubProduct.value("AdditionalPoints", 0);
						subProduct.additionalClanPoints = jSubProduct.value("AdditionalClanPoints", 0);
						subProduct.adType = jSubProduct.value("AdType", 0);
						product.subProducts.push_back(subProduct);
					}
				}
				m_Products.push_back(product);
			}
		}
	}
	catch (exception& ex)
	{
		Logger().Fatal("CShopManager::LoadProducts: an error occured while parsing Shop.json: %s\n", ex.what());
		return false;
	}

	return true;
}

// upgrade
bool CShopManager::KVToJson()
{
	struct SubProduct
	{
		int productId;
		int days;
		int count;
		int count2;
		int price;
		int additionalPoints;
		int additionalClanPoints;
		int adType;
	};

	struct Product
	{
		int itemId;
		bool isPoints;
		vector<SubProduct> subProducts;
	};
	vector<Product> m_Products;

	// check if exists
	ofstream f("Shop.json", ios::in);
	if (f.is_open())
		return false;

	f.close();

	{
		KV::KeyValues root = KV::KeyValues::parseFromFile("Shop.txt");
		if (root.isEmpty())
			return false;

		KV::KeyValues& shopProducts = root["ShopProducts"];

		stringstream iss(string(""));
		KV::KeyValues& kvRecommended = shopProducts["Recommended"];
		if (!kvRecommended.isEmpty())
		{
			for (auto& item : kvRecommended)
			{
				if (item.isSection())
					continue;

				vector<int> page;
				iss.str(item.getValue());
				page.assign((istream_iterator<int>(iss)), istream_iterator<int>());
				m_RecommendedProducts.push_back(page);
				iss.clear();
			}
		}

		KV::KeyValues& kvPopular = shopProducts["Popular"];
		iss.str(kvPopular.getValue());
		m_PopularProducts.assign((istream_iterator<int>(iss)), istream_iterator<int>());

		KV::KeyValues& kvProducts = shopProducts["Products"];
		if (!kvProducts.isEmpty())
		{
			int productTypeId = 1;
			for (auto& item : kvProducts)
			{
				if (!item.isSection())
					continue;

				Product product;
				product.itemId = stoi(item.getKey());
				product.isPoints = item["IsPoints"].getValueAsBool();

				KV::KeyValues& kvSubProducts = item["SubProducts"];
				if (kvSubProducts.isEmpty())
					continue;

				for (auto& kvSubProduct : kvSubProducts)
				{
					SubProduct subProduct;
					subProduct.productId = productTypeId++;
					subProduct.days = kvSubProduct["Days"].getValueAsInt();
					subProduct.count = kvSubProduct["Count"].getValueAsInt();
					subProduct.count2 = kvSubProduct["Count2"].getValueAsInt();
					subProduct.price = kvSubProduct["Price"].getValueAsInt();
					subProduct.additionalPoints = kvSubProduct["AdditionalPoints"].getValueAsInt();
					subProduct.additionalClanPoints = kvSubProduct["AdditionalClanPoints"].getValueAsInt();
					subProduct.adType = kvSubProduct["AdType"].getValueAsInt();

					product.subProducts.push_back(subProduct);
				}

				m_Products.push_back(product);
			}
		}
	}

	ordered_json cfg;
	cfg["Version"] = SHOP_JSON_VERSION;
	for (size_t i = 0; i < m_RecommendedProducts.size(); i++)
	{
		cfg["Recommended"][to_string(i)] = m_RecommendedProducts[i];
	}

	cfg["Popular"] = m_PopularProducts;

	for (size_t i = 0; i < m_Products.size(); i++)
	{
		Product product = m_Products[i];
		cfg["Products"][to_string(product.itemId)]["IsPoints"] = product.isPoints;

		for (size_t j = 0; j < product.subProducts.size(); j++)
		{
			cfg["Products"][to_string(product.itemId)]["SubProducts"][to_string(j)]["Count"] = product.subProducts[j].count;
			cfg["Products"][to_string(product.itemId)]["SubProducts"][to_string(j)]["Duration"] = product.subProducts[j].days;
			cfg["Products"][to_string(product.itemId)]["SubProducts"][to_string(j)]["Price"] = product.subProducts[j].price;
			cfg["Products"][to_string(product.itemId)]["SubProducts"][to_string(j)]["AdditionalPoints"] = product.subProducts[j].additionalPoints;
			cfg["Products"][to_string(product.itemId)]["SubProducts"][to_string(j)]["AdditionalClanPoints"] = product.subProducts[j].additionalClanPoints;
			cfg["Products"][to_string(product.itemId)]["SubProducts"][to_string(j)]["AdType"] = product.subProducts[j].adType;
		}
	}

	f.open("Shop.json");
	f << setfill('\t') << setw(1) << cfg << endl;

	return true;
}

void CShopManager::OnShopPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	IUser* user = g_UserManager.GetUserBySocket(socket);
	if (user == NULL)
		return;

	int type = msg->ReadUInt8();
	switch (type)
	{
	case ShopPacketType::RequestBuyProduct:
		BuyProduct(user, msg->ReadUInt8(), msg->ReadUInt8());
		break;
	}
}

void CShopManager::GetProductBySubId(int productId, Product& product, SubProduct& subProduct)
{
	for (auto& p : m_Products)
	{
		for (auto& sp : p.subProducts)
		{
			if (sp.productID == productId)
			{
				product = p;
				subProduct = sp;
				return;
			}
		}
	}
}

bool CShopManager::BuyProduct(IUser* user, int productTypeId, int productId)
{
	Product product = {};
	SubProduct subProduct = {};

	GetProductBySubId(productId, product, subProduct);

	if (!subProduct.productID)
	{
		// unknown sub product
		g_PacketManager.SendShopBuyProductReply(user->GetExtendedSocket(), ShopBuyProductReply::BUY_FAIL_NOITEM);
		return false;
	}

	if (product.isPoints)
	{
		CUserCharacter character = user->GetCharacter(UFLAG_POINTS);
		if (character.points < subProduct.price)
		{
			// not enough points
			g_PacketManager.SendShopBuyProductReply(user->GetExtendedSocket(), ShopBuyProductReply::BUY_FAIL_NO_POINT);
			return false;
		}
	}
	else
	{
		CUserCharacter character = user->GetCharacter(UFLAG_CASH);
		if (character.cash < subProduct.price)
		{
			g_PacketManager.SendShopBuyProductReply(user->GetExtendedSocket(), ShopBuyProductReply::BUY_FAIL_NO_POINT);
			return false;
		}
	}

	for (auto& item : subProduct.items)
	{
		int status = g_ItemManager.AddItem(user->GetID(), user, item.itemID, item.count, item.duration);
		switch (status)
		{
		case ITEM_ADD_INVENTORY_FULL:
			g_PacketManager.SendShopBuyProductReply(user->GetExtendedSocket(), ShopBuyProductReply::BUY_FAIL_INVENTORY_FULL);
			return false;
		case ITEM_ADD_UNKNOWN_ITEMID:
			g_PacketManager.SendShopBuyProductReply(user->GetExtendedSocket(), ShopBuyProductReply::BUY_FAIL_SYSTEM_ERROR);
			return false;
		}
	}

	if (product.isPoints)
	{
		user->UpdatePoints(-subProduct.price + subProduct.additionalPoints);
	}
	else
	{
		user->UpdateCash(-subProduct.price);

		if (subProduct.additionalPoints)
			user->UpdatePoints(subProduct.additionalPoints);
	}

	g_PacketManager.SendShopBuyProductReply(user->GetExtendedSocket(), ShopBuyProductReply::BUY_OK);

	return true;
}

const vector<Product>& CShopManager::GetProducts()
{
	return m_Products;
}

const vector<vector<int>>& CShopManager::GetRecommendedProducts()
{
	return m_RecommendedProducts;
}

const vector<int>& CShopManager::GetPopularProducts()
{
	return m_PopularProducts;
}