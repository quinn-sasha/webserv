/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config_utils.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ikota <ikota@student.42tokyo.jp>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 11:18:29 by ikota             #+#    #+#             */
/*   Updated: 2026/02/20 11:18:29 by ikota            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "config_utils.hpp"

void error_exit(const std::string& msg) {
	std::cerr << "Error: Config file: " << msg << std::endl;
	std::exit(1);
}

bool is_all_digits(const std::string& str) {
	if (str.empty())
		return false;
	for (size_t i = 0; i < str.size(); ++i) {
		if (!std::isdigit(str[i])) {
			return false;
		}
	}
	return true;
}

bool is_valid_ip(const std::string& str) {
    // TODO: IPアドレスのバリデーション　ドットで分割 .が３つあるか　各セグメントが空でないか、数字だけか　数値が0~255かどうか
    
}