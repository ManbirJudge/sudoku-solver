package com.example.sudokusolver

import android.content.Intent
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.net.toUri
import androidx.fragment.app.Fragment
import com.example.sudokusolver.databinding.FragmentAboutBinding
// import com.mikepenz.aboutlibraries.LibsBuilder

class AboutFragment : Fragment() {
    private lateinit var binding: FragmentAboutBinding

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
        binding = FragmentAboutBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        // binding.aboutOpenSourceBtn.setOnClickListener {
        //     LibsBuilder()
        //         .withShowLoadingProgress(true)
        //         .withSearchEnabled(true)
        //         .start(requireContext())
        // }

        binding.aboutDevBtn.setOnClickListener {
            startActivity(Intent(
                Intent.ACTION_VIEW,
                "https://manbir-judge.netlify.app".toUri()
            ))
        }
    }
}