#pragma once
#include <vector>
#include <stdint.h>
#include <limits>
#include <concepts>
#include <entt/entity/registry.hpp>

#include <concepts>
#include <iostream>

namespace Gawr {
	namespace ECS {		
		template<typename index_t>
		struct tombstone {
			inline static const index_t value = std::numeric_limits<index_t>::max();
			operator index_t() { return value; }
		};
		
		template<typename comp_t, typename entity_t = uint32_t> 
			requires std::is_integral_v<entity_t>
		class Storage {
		public:
			template<typename ... args_ts> requires std::is_constructible_v<comp_t, args_ts...>
			void emplace(entity_t e, args_ts&& ... args) {
				 
				uint32_t page_curr = get_page(e); // floor divide
				
				// if page within range
				if (page_curr >= m_pages.size())
					m_pages.resize(page_curr + 1, Page{ 0, 0 });

				// if page uninitiated
				if (m_pages[page_curr].empty())
				{
					// initiate page
					m_pages[page_curr].index = static_cast<uint32_t>(m_indices.size());
					m_pages[page_curr].count = 1u;
					
					m_indices.resize(m_indices.size() + Page::MaxSize, tombstone<uint32_t>{});
					m_page_no.push_back(page_curr);
				}
				else
				{
					m_pages[page_curr].count++; // add, no need to error check -> can never exceed page size
				}

				m_indices[get_index_in_page(e)] = static_cast<uint32_t>(m_entities.size());
				m_entities.push_back(e);
				m_components.emplace_back(std::move(args)...);
			}

			void erase(entity_t e) {
				uint32_t paged_i1 = get_index_in_page(e);
				uint32_t paged_i2 = get_index_in_page(m_entities.back());

				m_entities[m_indices[paged_i1]] = m_entities.back();
				m_entities.pop_back();
		
				m_components[m_indices[paged_i1]] = std::move(m_components.back());
				m_components.pop_back();

				m_indices[paged_i2] = m_indices[paged_i1];
				m_indices[paged_i1] = tombstone<uint32_t>{};

				uint32_t page_curr = get_page(e);
				m_pages[page_curr].count--;

				if (m_pages[page_curr].empty())
				{
					uint32_t page_back = m_page_no.back();

					if (page_back != page_curr)
					{
						std::memcpy(
							&m_indices[m_pages[page_curr].index],
							&m_indices[m_pages[page_back].index],
							m_pages[page_back].count * sizeof(uint32_t));

						
						m_page_no[m_pages[page_curr].index / Page::MaxSize] = page_back;

						m_pages[page_back].index = m_pages[page_curr].index;
						m_pages[page_curr] = { 0, 0 }; // set page to empty
					}
					
					m_page_no.pop_back();

					uint32_t i = static_cast<uint32_t>(m_pages.size());
					while (i > 0 && !(m_pages[--i].empty())) { 
						std::cout << i << "\n";
					}
					
					m_pages.resize(i);
					m_indices.resize(i * Page::MaxSize);
				}
		
			}

			void swap(uint32_t i1, uint32_t i2) {
				entity_t e1 = m_entities[i1];
				entity_t e2 = m_entities[i2];

				uint32_t paged_i1 = get_index_in_page(e1);
				uint32_t paged_i2 = get_index_in_page(e2);

				m_entities[i1] = e2;
				m_entities[i2] = e1;

				uint32_t temp =		  m_indices[paged_i1];
				m_indices[paged_i1] = m_indices[paged_i2];
				m_indices[paged_i2] = temp;
			}
			
			uint32_t index(entity_t e) {
				return m_indices[get_index_in_page(e)];
			}

			entity_t at(uint32_t i) {
				return m_entities[i];
			}

			comp_t& get(uint32_t i) {
				return &m_components[i];
			}

		private:
			inline uint32_t get_page(entity_t e) {
				return e / Page::MaxSize;
			}

			inline uint32_t get_index_in_page(entity_t e) {
				uint32_t page_index = e / Page::MaxSize;
				return m_pages[page_index].index + e - page_index * Page::MaxSize;
			}

			struct Page {
				uint32_t index;
				uint32_t count;

				const static uint32_t MaxSize = 128;

				bool empty() { return count == 0; }
			};
			
			std::vector<Page> m_pages;			// sparse array pages

			std::vector<uint32_t> m_page_no;	// packed array of page number
			std::vector<uint32_t> m_indices;	// sparsely paged array of index to entities
			
			std::vector<entity_t> m_entities;	// packed array of ordered entities
			std::vector<comp_t> m_components;	
		};
		
		/*
		template<typename entity_t>
		class View { 
			template<typename ... get_ts, typename ... exc_ts>
			View() {

			}

		};
		*/

		template<typename entity_t>
		class HandleManager {
			
		public:
			using index_t = entity_t;

			entity_t create() {
				entity_t e;
				if (!m_queue.empty()) {
					e = m_queue.back();
					m_queue.pop_back();
				}
				else {
					e = m_next;
					m_next++;
				}
				assure(e);
				m_entities[e] = true;
				return e;
			}
			
			void destroy(entity_t e) {
				if (e >= m_entities.size()) return;

				m_entities[e] = false;

				if (m_next - 1 == e)
					m_next = e;
				else
					m_queue.push_back(e);
			}
			
			bool valid(entity_t e) {
				return e < m_entities.size() && m_entities[e];
			}

		private:
			std::vector<bool>	  m_entities;
			std::vector<entity_t> m_queue;
			entity_t			  m_next;
		};
		
		template<typename ... comp_ts>
		class Registry {
		public:
			using entity_t = uint32_t;

			entity_t create() {
				m_handleManager.create();
			}

			template<typename comp_t, typename ... args_ts>
			void emplace(entity_t e, args_ts&& ... args) {
				storage<comp_t>().emplace(std::move(args)...);
			}
			
			template<typename comp_t>
			Storage<comp_t, entity_t>& storage() {
				return std::get<Storage<comp_t, entity_t>>(m_pools);
			}

		private:
			std::tuple<Storage<comp_ts, entity_t>...> m_pools;
			HandleManager<entity_t> m_handleManager;
		};


	}
}
